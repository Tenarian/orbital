#pragma once

#include <QObject>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QLoggingCategory>

#include "orbital/paths.h"
#include "orbital/types.h"

Q_DECLARE_LOGGING_CATEGORY(lcConfigStore)

namespace Orbital {

// ConfigStore — simple load/save helper for the global AppConfig stored at
// ~/.config/orbital/config.json (XDG_CONFIG_HOME/orbital/config.json).
//
// GameLibrary no longer touches this file — it persists per-game metadata at
// games/by-uuid/{uuid}/metadata.json instead (see PRD §XDG Directory Layout).
class ConfigStore : public QObject {
    Q_OBJECT

public:
    explicit ConfigStore(QObject *parent = nullptr) : QObject(parent) {}

    // Load config from disk; returns default-constructed AppConfig on missing
    // or invalid file (with a warning logged).
    AppConfig load() const {
        const QString path = Paths::configFile();
        QFile f(path);
        if (!f.exists()) {
            return AppConfig{};
        }
        if (!f.open(QIODevice::ReadOnly)) {
            qCWarning(lcConfigStore) << "Cannot open" << path << ":" << f.errorString();
            return AppConfig{};
        }
        QJsonParseError err{};
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        f.close();
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qCWarning(lcConfigStore) << "Invalid config at" << path << ":" << err.errorString();
            return AppConfig{};
        }
        return AppConfig::fromJson(doc.object());
    }

    // Save config atomically (QSaveFile manages the .tmp internally and
    // renames on commit()).
    void save(const AppConfig &config) const {
        const QString path = Paths::configFile();
        QDir().mkpath(QFileInfo(path).absolutePath());

        QSaveFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(lcConfigStore) << "Cannot open" << path << ":" << f.errorString();
            return;
        }
        f.write(QJsonDocument(config.toJson()).toJson(QJsonDocument::Indented));
        if (!f.commit()) {
            qCWarning(lcConfigStore) << "Atomic write failed for" << path << ":" << f.errorString();
        }
    }
};

} // namespace Orbital
