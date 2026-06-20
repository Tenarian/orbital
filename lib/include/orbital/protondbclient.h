#pragma once

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "orbital/paths.h"
#include "orbital/types.h"

namespace Orbital {

enum class ProtonTier {
    Borked,
    Bronze,
    Silver,
    Gold,
    Platinum,
    Native
};

struct ProtonDBResult {
    QString tier;           // "platinum", "gold", etc.
    int ratingPercentage;
    int totalReports;
    QString playtime;
    QString os;              // "linux", "windows"
};

class ProtonDBClient : public QObject {
    Q_OBJECT

public:
    explicit ProtonDBClient(QObject *parent = nullptr);
    ~ProtonDBClient() override;

    // Fetch ProtonDB summary for a Steam App ID
    ProtonDBResult fetch(int steamAppId);

    // Get cached result
    ProtonDBResult cached(int steamAppId) const;

    // Suggest Proton version based on game name
    QString suggestProton(const QString &gameName) const;

    // Check if result is cached and still valid (TTL 7 days)
    bool isCacheValid(int steamAppId) const;

signals:
    void fetchFinished(int steamAppId, const ProtonDBResult &result);
    void fetchError(int steamAppId, const QString &error);

private slots:
    void onNetworkReply();

private:
    QNetworkAccessManager m_network;
    QString cachePath(int steamAppId) const;
    ProtonDBResult parseResult(const QJsonObject &obj) const;
    ProtonTier parseTier(const QString &tier) const;
};

} // namespace Orbital

Q_DECLARE_METATYPE(Orbital::ProtonTier)
