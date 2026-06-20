#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "orbital/types.h"
#include "orbital/paths.h"

namespace Orbital {

struct Preset {
    QString id;
    QString label;
    QString description;
    QString protonVersion;
    QMap<QString, QString> envVars;
    QStringList tags;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["label"] = label;
        obj["description"] = description;
        obj["protonVersion"] = protonVersion;
        QJsonArray tagsArr;
        for (const auto &t : tags) tagsArr.append(t);
        obj["tags"] = tagsArr;
        QJsonObject env;
        for (auto it = envVars.constBegin(); it != envVars.constEnd(); ++it) {
            env[it.key()] = it.value();
        }
        obj["envVars"] = env;
        return obj;
    }

    static Preset fromJson(const QJsonObject &obj) {
        Preset preset;
        preset.id = obj["id"].toString();
        preset.label = obj["label"].toString();
        preset.description = obj["description"].toString();
        preset.protonVersion = obj["protonVersion"].toString();
        QJsonArray tagsArr = obj["tags"].toArray();
        for (auto it = tagsArr.constBegin(); it != tagsArr.constEnd(); ++it) {
            preset.tags.append(it->toString());
        }
        QJsonObject env = obj["envVars"].toObject();
        for (auto it = env.constBegin(); it != env.constEnd(); ++it) {
            preset.envVars[it.key()] = it.value().toString();
        }
        return preset;
    }
};

struct PresetIndexEntry {
    QString name;
    QString steamAppId;
    QString updatedAt;
    QStringList contributors;
    QStringList tags;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["steamAppId"] = steamAppId;
        obj["updatedAt"] = updatedAt;
        QJsonArray contrib;
        for (const auto &c : contributors) contrib.append(c);
        obj["contributors"] = contrib;
        QJsonArray tagsArr;
        for (const auto &t : tags) tagsArr.append(t);
        obj["tags"] = tagsArr;
        return obj;
    }

    static PresetIndexEntry fromJson(const QJsonObject &obj) {
        PresetIndexEntry entry;
        entry.name = obj["name"].toString();
        entry.steamAppId = obj["steamAppId"].toString();
        entry.updatedAt = obj["updatedAt"].toString();
        QJsonArray contribArr = obj["contributors"].toArray();
        for (auto it = contribArr.constBegin(); it != contribArr.constEnd(); ++it) {
            entry.contributors.append(it->toString());
        }
        QJsonArray tagsArr = obj["tags"].toArray();
        for (auto it = tagsArr.constBegin(); it != tagsArr.constEnd(); ++it) {
            entry.tags.append(it->toString());
        }
        return entry;
    }
};

class PresetManager : public QObject {
    Q_OBJECT

public:
    explicit PresetManager(QObject *parent = nullptr);
    ~PresetManager() override;

    // Initialize: load cache, check for refresh
    void init();

    // Get all presets for a game
    QList<Preset> presetsForGame(const QString &gameName) const;

    // Get all presets for a Steam App ID
    QList<Preset> presetsForSteamAppId(const QString &steamAppId) const;

    // Refresh cache from remote
    void refresh();

    // Save a local preset for a game
    void saveLocalPreset(const QString &gameName, const Preset &preset);

    // Get local presets for a game
    QList<Preset> localPresetsForGame(const QString &gameName) const;

    // Delete a local preset
    void deleteLocalPreset(const QString &gameName, const QString &presetId);

    // Merge multiple presets with conflict resolution
    QList<std::pair<QString, QString>> mergePresets(
        const QList<Preset> &presets,
        const QList<int> &selectedIndices) const;

    // Check if cache needs refresh (> 24h old)
    bool needsRefresh() const;

signals:
    void presetsLoaded(const QString &gameName, const QList<Preset> &presets);
    void cacheRefreshed();
    void cacheError(const QString &error);

private slots:
    void onNetworkReply();

private:
    QNetworkAccessManager m_network;
    QList<PresetIndexEntry> m_index;
    QMap<QString, QList<Preset>> m_cache;  // gameName -> presets

    QString indexCachePath() const;
    QString presetFileForGame(const QString &gameName) const;
    bool loadIndex();
    bool saveIndex(const QList<PresetIndexEntry> &index);
    void fetchRemoteIndex();
    void fetchGamePresets(const QString &gameSlug);
    bool isIndexStale() const;
    qint64 indexAge() const;
};

} // namespace Orbital
