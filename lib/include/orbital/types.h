#pragma once

#include <QString>
#include <QMap>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Orbital {

// ============================================================================
// EnvVarSource — where an environment variable originated
// ============================================================================

enum class EnvVarSource {
    Generated,  // From schema GUI config
    Custom      // Manually entered by user
};

// ============================================================================
// EnvVarState — current state of an environment variable
// ============================================================================

enum class EnvVarState {
    Clean,      // GUI and Env Vars tab are in sync
    Modified,   // User edited manually, value still parses
    Invalid     // User edited manually, value fails to parse
};

// ============================================================================
// EnvVarEntry — a single environment variable entry
// ============================================================================

struct EnvVarEntry {
    QString key;
    QString value;           // raw user input, never normalized
    EnvVarSource source;
    EnvVarState state;
    QString categoryId;      // if source == Generated
    QString groupId;         // if part of an env-group, the virtual key
    bool locked = false;     // true → shown with 🔒 in Env Vars tab
    QString errorMessage;

    EnvVarEntry()
        : source(EnvVarSource::Generated)
        , state(EnvVarState::Clean)
        , locked(false)
    {}

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["key"] = key;
        obj["value"] = value;
        obj["source"] = source == EnvVarSource::Generated ? "generated" : "custom";
        obj["state"] = state == EnvVarState::Clean ? "clean"
                   : state == EnvVarState::Modified ? "modified" : "invalid";
        if (!categoryId.isEmpty()) obj["categoryId"] = categoryId;
        if (!groupId.isEmpty()) obj["groupId"] = groupId;
        obj["locked"] = locked;
        if (!errorMessage.isEmpty()) obj["errorMessage"] = errorMessage;
        return obj;
    }

    static EnvVarEntry fromJson(const QJsonObject &obj) {
        EnvVarEntry entry;
        entry.key = obj["key"].toString();
        entry.value = obj["value"].toString();
        QString src = obj["source"].toString();
        entry.source = src == "custom" ? EnvVarSource::Custom : EnvVarSource::Generated;
        QString st = obj["state"].toString();
        if (st == "modified") entry.state = EnvVarState::Modified;
        else if (st == "invalid") entry.state = EnvVarState::Invalid;
        else entry.state = EnvVarState::Clean;
        entry.categoryId = obj["categoryId"].toString();
        entry.groupId = obj["groupId"].toString();
        entry.locked = obj["locked"].toBool(false);
        entry.errorMessage = obj["errorMessage"].toString();
        return entry;
    }

    bool operator==(const EnvVarEntry &other) const {
        return key == other.key
            && value == other.value
            && source == other.source
            && state == other.state
            && categoryId == other.categoryId
            && groupId == other.groupId
            && locked == other.locked
            && errorMessage == other.errorMessage;
    }

    bool operator!=(const EnvVarEntry &other) const {
        return !(*this == other);
    }
};

// ============================================================================
// ParseResult — result of parsing an env var value
// ============================================================================

struct ParseResult {
    bool valid;
    QVariant parsedValue;
    QString errorMessage;

    ParseResult() : valid(false) {}
    explicit ParseResult(bool v, const QString &err = {})
        : valid(v), errorMessage(err) {}
};

// ============================================================================
// LaunchConfig — configuration for launching a game
// ============================================================================

struct LaunchConfig {
    QString executable;
    QStringList args;
    QString workingDir;
    QString protonPath;
    QString prefixPath;
    QMap<QString, QString> envVars;
    bool skipEnvValidation = false;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["executable"] = executable;
        obj["workingDir"] = workingDir;
        obj["protonPath"] = protonPath;
        obj["prefixPath"] = prefixPath;
        obj["skipEnvValidation"] = skipEnvValidation;
        QJsonObject env;
        for (auto it = envVars.constBegin(); it != envVars.constEnd(); ++it) {
            env[it.key()] = it.value();
        }
        obj["envVars"] = env;
        return obj;
    }

    static LaunchConfig fromJson(const QJsonObject &obj) {
        LaunchConfig config;
        config.executable = obj["executable"].toString();
        config.workingDir = obj["workingDir"].toString();
        config.protonPath = obj["protonPath"].toString();
        config.prefixPath = obj["prefixPath"].toString();
        config.skipEnvValidation = obj["skipEnvValidation"].toBool(false);
        QJsonObject env = obj["envVars"].toObject();
        for (auto it = env.constBegin(); it != env.constEnd(); ++it) {
            config.envVars[it.key()] = it.value().toString();
        }
        return config;
    }
};

// ============================================================================
// HLTBData — HowLongToBeat data
// ============================================================================

struct HLTBData {
    int mainSeconds;
    int mainPlusSeconds;
    int completionSeconds;

    HLTBData()
        : mainSeconds(0)
        , mainPlusSeconds(0)
        , completionSeconds(0)
    {}

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["mainSeconds"] = mainSeconds;
        obj["mainPlusSeconds"] = mainPlusSeconds;
        obj["completionSeconds"] = completionSeconds;
        return obj;
    }

    static HLTBData fromJson(const QJsonObject &obj) {
        HLTBData data;
        data.mainSeconds = obj["mainSeconds"].toInt(0);
        data.mainPlusSeconds = obj["mainPlusSeconds"].toInt(0);
        data.completionSeconds = obj["completionSeconds"].toInt(0);
        return data;
    }
};

// ============================================================================
// GameEntry — a single game entry in the library
// ============================================================================

struct GameEntry {
    QString id;
    QString name;
    QString executable;
    QString protonVersion;
    QString launchArgs;
    qint64 lastPlayed = 0;
    bool suppressEnvWarning = false;
    QString prefixMode;  // "per-game" or "shared"
    QString sharedPrefixId;
    QMap<QString, QString> envVars;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["executable"] = executable;
        obj["protonVersion"] = protonVersion;
        obj["launchArgs"] = launchArgs;
        obj["lastPlayed"] = lastPlayed;
        obj["suppressEnvWarning"] = suppressEnvWarning;
        obj["prefixMode"] = prefixMode;
        if (!sharedPrefixId.isEmpty()) {
            obj["sharedPrefixId"] = sharedPrefixId;
        } else {
            obj["sharedPrefixId"] = QJsonValue::Null;
        }
        QJsonObject env;
        for (auto it = envVars.constBegin(); it != envVars.constEnd(); ++it) {
            env[it.key()] = it.value();
        }
        obj["envVars"] = env;
        return obj;
    }

    static GameEntry fromJson(const QJsonObject &obj) {
        GameEntry entry;
        entry.id = obj["id"].toString();
        entry.name = obj["name"].toString();
        entry.executable = obj["executable"].toString();
        entry.protonVersion = obj["protonVersion"].toString();
        entry.launchArgs = obj["launchArgs"].toString();
        entry.lastPlayed = obj["lastPlayed"].toInteger(0);
        entry.suppressEnvWarning = obj["suppressEnvWarning"].toBool(false);
        entry.prefixMode = obj["prefixMode"].toString("per-game");
        entry.sharedPrefixId = obj["sharedPrefixId"].toString();
        QJsonObject env = obj["envVars"].toObject();
        for (auto it = env.constBegin(); it != env.constEnd(); ++it) {
            entry.envVars[it.key()] = it.value().toString();
        }
        return entry;
    }
};

// ============================================================================
// Config — global application config
// ============================================================================

struct AppConfig {
    bool suppressEnvWarning = false;
    bool sidebarCollapsed = false;
    QString steamGridDbApiKey;
    bool protonCheckUpdatesOnStartup = true;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["suppressEnvWarning"] = suppressEnvWarning;
        obj["sidebarCollapsed"] = sidebarCollapsed;
        if (!steamGridDbApiKey.isEmpty()) {
            obj["steamGridDbApiKey"] = steamGridDbApiKey;
        }
        obj["protonCheckUpdatesOnStartup"] = protonCheckUpdatesOnStartup;
        return obj;
    }

    static AppConfig fromJson(const QJsonObject &obj) {
        AppConfig config;
        config.suppressEnvWarning = obj["suppressEnvWarning"].toBool(false);
        config.sidebarCollapsed = obj["sidebarCollapsed"].toBool(false);
        config.steamGridDbApiKey = obj["steamGridDbApiKey"].toString();
        config.protonCheckUpdatesOnStartup = obj["protonCheckUpdatesOnStartup"].toBool(true);
        return config;
    }
};

// ============================================================================
// PrefixMeta — metadata for a shared prefix
// ============================================================================

struct PrefixMeta {
    QString id;
    QString name;
    QString description;
    QString protonVersion;
    QStringList games;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["description"] = description;
        obj["protonVersion"] = protonVersion;
        QJsonArray arr;
        for (const auto &g : games) {
            arr.append(g);
        }
        obj["games"] = arr;
        return obj;
    }

    static PrefixMeta fromJson(const QJsonObject &obj) {
        PrefixMeta meta;
        meta.id = obj["id"].toString();
        meta.name = obj["name"].toString();
        meta.description = obj["description"].toString();
        meta.protonVersion = obj["protonVersion"].toString();
        QJsonArray arr = obj["games"].toArray();
        for (auto it = arr.constBegin(); it != arr.constEnd(); ++it) {
            meta.games.append(it->toString());
        }
        return meta;
    }
};

} // namespace Orbital

Q_DECLARE_METATYPE(Orbital::EnvVarSource)
Q_DECLARE_METATYPE(Orbital::EnvVarState)
Q_DECLARE_METATYPE(Orbital::GameEntry)
Q_DECLARE_METATYPE(Orbital::LaunchConfig)
Q_DECLARE_METATYPE(Orbital::EnvVarEntry)
