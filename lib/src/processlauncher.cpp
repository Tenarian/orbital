#include "orbital/processlauncher.h"
#include "orbital/paths.h"
#include "orbital/envvarschema.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QStandardPaths>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcProcessLauncher, "orbital.processlauncher")

namespace Orbital {

// ============================================================================
// ctor / dtor
// ============================================================================

ProcessLauncher::ProcessLauncher(QObject *parent) : QObject(parent) {
    m_process = new QProcess(this);
    connect(m_process, &QProcess::started, this, &ProcessLauncher::onStarted);
    connect(m_process, &QProcess::finished, this, &ProcessLauncher::onFinished);
    connect(m_process, &QProcess::errorOccurred, this, &ProcessLauncher::onErrorOccurred);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &ProcessLauncher::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &ProcessLauncher::onReadyReadStandardError);
}

ProcessLauncher::~ProcessLauncher() = default;

// ============================================================================
// launch-filter.json — system env deny-list, loaded from disk.
//
// Per PRD §ProcessLauncher: filter lives in assets/schema/launch-filter.json
// so users can edit it without recompiling. Default deny-list:
//   LD_PRELOAD, LD_LIBRARY_PATH, WINEPREFIX, WINEFSYNC, WINEESYNC
//
// NOTE: we deliberately do NOT strip DISPLAY/HOME/USER/PATH/LANG — games
// need them to run.
// ============================================================================

QStringList ProcessLauncher::parseLaunchFilter(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qCDebug(lcProcessLauncher) << "Cannot read launch-filter.json at" << path;
        return {};
    }
    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qCWarning(lcProcessLauncher) << "Invalid launch-filter.json:" << err.errorString();
        return {};
    }
    const QJsonArray arr = doc.object().value("denyList").toArray();
    QStringList result;
    for (const auto &v : arr) result.append(v.toString());
    return result;
}

QStringList ProcessLauncher::loadFilterDenyList() const {
    if (m_filterLoaded) return m_filterDenyList;

    // Try user override first, then built-in.
    const QStringList candidates = {
        Paths::userSchemas() + "/launch-filter.json",
        Paths::builtinSchemas() + "/launch-filter.json",
    };
    for (const QString &path : candidates) {
        QStringList list = parseLaunchFilter(path);
        if (!list.isEmpty()) {
            m_filterDenyList = list;
            m_filterLoaded = true;
            return m_filterDenyList;
        }
    }

    // Hardcoded fallback (matches PRD default).
    m_filterDenyList = {
        "LD_PRELOAD",
        "LD_LIBRARY_PATH",
        "WINEPREFIX",
        "WINEFSYNC",
        "WINEESYNC",
    };
    m_filterLoaded = true;
    return m_filterDenyList;
}

// ============================================================================
// expandEnvGroups — expand virtual env-group keys into their member vars.
//
// For each env-group virtual key present in envVars with a truthy value
// ("1", "true", "yes", "on"), expand into all member vars from the schema.
// Locked members get their fixed value; non-locked members keep whatever
// the user set in envVars (or skip if unset).
//
// Returns a NEW map with virtual keys removed and member vars added.
// If no schema is wired, returns the input unchanged.
// ============================================================================

QMap<QString, QString> ProcessLauncher::expandEnvGroups(const QMap<QString, QString> &envVars) const {
    if (!m_schema) return envVars;

    QMap<QString, QString> result = envVars;
    const QStringList groupIds = m_schema->envGroupIds();

    for (const QString &groupId : groupIds) {
        if (!result.contains(groupId)) continue;

        const QString val = result.value(groupId).trimmed().toLower();
        const bool enabled = (val == "1" || val == "true" || val == "yes" || val == "on");
        result.remove(groupId);  // virtual key is never injected

        if (!enabled) continue;

        const QList<PropertyDef::GroupMember> members = m_schema->envGroupMemberDefs(groupId);
        for (const PropertyDef::GroupMember &m : members) {
            if (!m.fixedValue.isEmpty()) {
                // Locked member: always use fixed value (overrides user).
                result.insert(m.key, m.fixedValue);
            } else if (!result.contains(m.key)) {
                // Non-locked member: leave user value alone, or skip if unset.
                // (We do NOT inject empty defaults.)
            }
            // If the user already set the member var explicitly, keep it.
        }
    }

    return result;
}

// ============================================================================
// buildEnvironment — layer env in the order specified by PRD §ProcessLauncher.
//
//   1. System env (filtered via launch-filter.json deny-list)
//   2. STEAM_COMPAT_DATA_PATH + WINEPREFIX (from prefixPath)
//   3. Game envVars (raw user values)
//   4. env-group expansion (virtual keys → real vars)
//
// Later entries win, so the final map has group expansion on top.
// ============================================================================

QProcessEnvironment ProcessLauncher::buildEnvOrdered(const LaunchConfig &config) const {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // 1. Strip deny-listed vars from system env.
    const QStringList deny = loadFilterDenyList();
    for (const QString &k : deny) {
        env.remove(k);
    }

    // 2. Steam compat paths from prefixPath.
    if (!config.prefixPath.isEmpty()) {
        env.insert("WINEPREFIX", config.prefixPath);
        // STEAM_COMPAT_DATA_PATH is normally the prefix's parent compatdata dir.
        env.insert("STEAM_COMPAT_DATA_PATH", config.prefixPath);
    }

    // 3. Game envVars (raw user values — no normalization per PRD).
    // 4. env-group expansion (virtual keys → real vars).
    QMap<QString, QString> expanded = expandEnvGroups(config.envVars);
    for (auto it = expanded.constBegin(); it != expanded.constEnd(); ++it) {
        env.insert(it.key(), it.value());
    }

    return env;
}

QProcessEnvironment ProcessLauncher::buildEnvironment(const LaunchConfig &config) const {
    return buildEnvOrdered(config);
}

// ============================================================================
// validateEnvVars — returns true if no env var is in the Invalid state.
// (Caller is responsible for tracking per-var state; here we just do basic
// schema validation if a schema is wired.)
// ============================================================================

bool ProcessLauncher::validateEnvVars(const QMap<QString, QString> &envVars) const {
    if (!m_schema) return true;
    for (auto it = envVars.constBegin(); it != envVars.constEnd(); ++it) {
        // Skip env-group virtual keys — they are validated separately.
        if (m_schema->envGroupIds().contains(it.key())) continue;
        const ParseResult r = m_schema->validate(it.key(), it.value());
        if (!r.valid) {
            qCWarning(lcProcessLauncher) << "Invalid env var" << it.key() << "=" << it.value()
                                          << ":" << r.errorMessage;
            return false;
        }
    }
    return true;
}

// ============================================================================
// launch — actually start the game via Proton.
//
// Per PRD: `proc->setProgram(config.protonPath); proc->setArguments({"run", config.executable} + config.args);`
// ============================================================================

bool ProcessLauncher::launch(const LaunchConfig &config) {
    if (config.protonPath.isEmpty()) {
        qCWarning(lcProcessLauncher) << "Cannot launch: protonPath is empty";
        emit processError(QProcess::FailedToStart);
        return false;
    }
    if (config.executable.isEmpty()) {
        qCWarning(lcProcessLauncher) << "Cannot launch: executable is empty";
        emit processError(QProcess::FailedToStart);
        return false;
    }

    // Validate env vars (unless caller explicitly skipped).
    if (!config.skipEnvValidation) {
        if (!validateEnvVars(config.envVars)) {
            qCWarning(lcProcessLauncher) << "Refusing to launch: invalid env vars present";
            emit processError(QProcess::FailedToStart);
            return false;
        }
    }

    // Build env.
    const QProcessEnvironment env = buildEnvOrdered(config);
    m_process->setProcessEnvironment(env);

    // Working directory.
    if (!config.workingDir.isEmpty()) {
        m_process->setWorkingDirectory(config.workingDir);
    } else {
        // Default to the executable's directory.
        m_process->setWorkingDirectory(QFileInfo(config.executable).absolutePath());
    }

    // Program: the Proton binary. Arguments: "run" <executable> [args...]
    m_process->setProgram(config.protonPath);
    QStringList args;
    args << "run" << config.executable;
    args << config.args;
    m_process->setArguments(args);

    qCInfo(lcProcessLauncher) << "Launching:" << config.protonPath << args;
    m_process->start();
    return true;
}

bool ProcessLauncher::launchGame(const QString &gameId, const QString &protonPath,
                                  const QString &prefixPath) {
    Q_UNUSED(gameId);
    Q_UNUSED(protonPath);
    Q_UNUSED(prefixPath);
    // Without a GameLibrary reference here, this is a thin shim.
    // Callers (CLI) resolve the GameEntry themselves and call launch() directly.
    qCWarning(lcProcessLauncher) << "launchGame(id) is deprecated — use launch(LaunchConfig) directly";
    return false;
}

// ============================================================================
// QProcess signal handlers
// ============================================================================

void ProcessLauncher::onStarted() {
    qCInfo(lcProcessLauncher) << "Process started, PID:" << m_process->processId();
    emit processStarted();
}

void ProcessLauncher::onFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qCInfo(lcProcessLauncher) << "Process finished, exitCode:" << exitCode << "status:" << exitStatus;
    emit processFinished(exitCode, exitStatus);
}

void ProcessLauncher::onErrorOccurred(QProcess::ProcessError error) {
    qCWarning(lcProcessLauncher) << "Process error:" << error << "-" << m_process->errorString();
    emit processError(error);
}

void ProcessLauncher::onReadyReadStandardOutput() {
    const QString out = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!out.isEmpty()) emit processOutput(out);
}

void ProcessLauncher::onReadyReadStandardError() {
    const QString err = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!err.isEmpty()) emit processOutput(err);
}

} // namespace Orbital
