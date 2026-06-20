#include "orbital/protonmanager.h"
#include "orbital/paths.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcProtonManager, "orbital.protonmanager")

namespace Orbital {

ProtonManager::ProtonManager(QObject *parent) : QObject(parent) {}

ProtonManager::~ProtonManager() = default;

QString ProtonManager::protonDir() const {
    // Search for ProtonGE or Proton in the steamapps/common directory
    QString steamDir = Paths::steamDir();
    if (steamDir.isEmpty()) {
        steamDir = QDir::home().absolutePath() + "/.local/share/Steam";
    }

    QDir commonDir(steamDir + "/steamapps/common");
    if (commonDir.exists()) {
        QStringList protonDirs = commonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto &dir : protonDirs) {
            if (dir.contains("Proton", Qt::CaseInsensitive)) {
                return commonDir.absolutePath() + "/" + dir;
            }
        }
    }

    return QString();
}

QList<QString> ProtonManager::installedVersions() const {
    QString protonDirPath = protonDir();
    QList<QString> versions;
    if (!protonDirPath.isEmpty()) {
        QDir dir(protonDirPath);
        for (const auto &entry : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (entry.fileName().startsWith("Proton")) {
                versions.append(entry.fileName());
            }
        }
    }
    return versions;
}

QString ProtonManager::latestVersion() const {
    auto versions = installedVersions();
    if (versions.isEmpty()) {
        return {};
    }
    return versions.last();
}

QString ProtonManager::suggestVersion(const QString &gameName) const {
    Q_UNUSED(gameName);
    auto versions = installedVersions();
    if (versions.isEmpty()) {
        return {};
    }
    return versions.first();
}

void ProtonManager::downloadVersion(const QString &version) {
    Q_UNUSED(version);
    // TODO: Download from GitHub
    qCInfo(lcProtonManager) << "Download requested:" << version;
}

void ProtonManager::deleteVersion(const QString &version) {
    Q_UNUSED(version);
    // TODO: Remove proton directory
    qCInfo(lcProtonManager) << "Delete requested:" << version;
}

void ProtonManager::onNetworkReply() {
    // TODO: Handle network reply
    qCDebug(lcProtonManager) << "Network reply handler (stub)";
}

} // namespace Orbital
