#include "orbital/protondbclient.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcProtonDB, "orbital.protondb")

namespace Orbital {

ProtonDBClient::ProtonDBClient(QObject *parent)
    : QObject(parent)
{
}

ProtonDBClient::~ProtonDBClient() = default;

ProtonDBResult ProtonDBClient::fetch(int steamAppId) {
    Q_UNUSED(steamAppId);
    // TODO: Fetch from ProtonDB API
    return ProtonDBResult{"platinum", 0, 0, "", "linux"};
}

ProtonDBResult ProtonDBClient::cached(int steamAppId) const {
    Q_UNUSED(steamAppId);
    return ProtonDBResult{"platinum", 0, 0, "", "linux"};
}

QString ProtonDBClient::suggestProton(const QString &gameName) const {
    Q_UNUSED(gameName);
    return "GE-Proton9-12";
}

bool ProtonDBClient::isCacheValid(int steamAppId) const {
    Q_UNUSED(steamAppId);
    return false;
}

QString ProtonDBClient::cachePath(int steamAppId) const {
    return QDir::home().absoluteFilePath(
        ".local/share/orbital/cache/protondb_" + QString::number(steamAppId) + ".json");
}

ProtonDBResult ProtonDBClient::parseResult(const QJsonObject &obj) const {
    ProtonDBResult result;
    result.tier = obj["tier"].toString("platinum");
    result.ratingPercentage = obj["ratingPercentage"].toInt(0);
    result.totalReports = obj["totalReports"].toInt(0);
    result.playtime = obj["playtime"].toString();
    result.os = obj["os"].toString("linux");
    return result;
}

ProtonTier ProtonDBClient::parseTier(const QString &tier) const {
    if (tier == "platinum") return ProtonTier::Platinum;
    if (tier == "gold") return ProtonTier::Gold;
    if (tier == "silver") return ProtonTier::Silver;
    if (tier == "bronze") return ProtonTier::Bronze;
    if (tier == "borked") return ProtonTier::Borked;
    return ProtonTier::Native;
}

void ProtonDBClient::onNetworkReply() {
    // TODO: Implement network reply handler
}

} // namespace Orbital
