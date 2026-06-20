#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "orbital/paths.h"

namespace Orbital {

class ProtonManager : public QObject {
    Q_OBJECT

public:
    explicit ProtonManager(QObject *parent = nullptr);
    ~ProtonManager() override;

    // Get all installed Proton versions
    QStringList installedVersions() const;

    // Get the Proton base directory
    QString protonDir() const;

    // Get latest GE-Proton version from GitHub
    QString latestVersion() const;

    // Download a Proton version
    void downloadVersion(const QString &version);

    // Delete a Proton version
    void deleteVersion(const QString &version);

    // Suggest Proton version based on game name (via ProtonDB)
    QString suggestVersion(const QString &gameName) const;

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString &version);
    void downloadError(const QString &error);
    void versionChecked(bool hasUpdate);

private slots:
    void onNetworkReply();

private:
    QNetworkAccessManager m_network;
    QStringList m_installed;
    QString m_latestVersion;
};

} // namespace Orbital
