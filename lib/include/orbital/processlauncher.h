#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcessEnvironment>
#include <QProcess>

#include "orbital/types.h"
#include "orbital/paths.h"
#include "orbital/envvarschema.h"

namespace Orbital {

class ProcessLauncher : public QObject {
    Q_OBJECT

public:
    explicit ProcessLauncher(QObject *parent = nullptr);
    ~ProcessLauncher() override;

    // Launch a game with the given config. Spawns QProcess asynchronously.
    // Returns true if the process was started successfully; the processFinished
    // / processError signals fire on completion / failure.
    bool launch(const LaunchConfig &config);

    // Convenience: launch with a game ID. Resolves protonPath/prefixPath/envVars
    // from the GameLibrary entry. If protonPath is empty, falls back to the
    // game's `protonVersion` field — but note that resolving a Proton install
    // path from a version string is the job of ProtonManager (not yet wired
    // here; pass an absolute path instead).
    bool launchGame(const QString &gameId, const QString &protonPath = {},
                    const QString &prefixPath = {});

    // Check if env vars are valid (no Invalid state). Used to decide whether
    // to show the "Invalid env vars" warning dialog before launch.
    bool validateEnvVars(const QMap<QString, QString> &envVars) const;

    // Build the final process environment for a launch. Filter system env
    // with the launch-filter deny-list, then layer on (in order, later wins):
    //   1. Filtered system env
    //   2. STEAM_COMPAT_DATA_PATH + WINEPREFIX (from prefixPath)
    //   3. Game envVars (raw user values)
    //   4. env-group expansion (virtual keys → real vars)
    QProcessEnvironment buildEnvironment(const LaunchConfig &config) const;

    // Get the QProcess pointer (for monitoring).
    QProcess *process() const { return m_process; }

    // Expand env-group virtual keys. For each virtual key present in envVars
    // with a "truthy" value, expands into the underlying member vars from
    // the schema. Locked member vars get their fixed value; non-locked member
    // vars keep whatever the user set (or skip if unset).
    QMap<QString, QString> expandEnvGroups(const QMap<QString, QString> &envVars) const;

    // Wire an EnvVarSchemaLoader into this launcher (needed for env-group
    // expansion). Caller retains ownership.
    void setSchemaLoader(EnvVarSchemaLoader *loader) { m_schema = loader; }

signals:
    void processStarted();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);
    void processOutput(const QString &output);

private:
    QProcess *m_process = nullptr;
    EnvVarSchemaLoader *m_schema = nullptr;
    mutable QStringList m_filterDenyList;
    mutable bool m_filterLoaded = false;

    // Load the launch-filter.json deny-list from disk (cached after first call).
    QStringList loadFilterDenyList() const;

    // Build the ordered env (the implementation behind buildEnvironment).
    QProcessEnvironment buildEnvOrdered(const LaunchConfig &config) const;

    // Parse launch-filter.json (lives in assets/schema/launch-filter.json).
    static QStringList parseLaunchFilter(const QString &path);

private slots:
    void onStarted();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onErrorOccurred(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
};

} // namespace Orbital
