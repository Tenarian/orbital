#include "bridge/protonmodel.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcProtonModel, "orbital.protonmodel")

namespace Orbital {

ProtonModel::ProtonModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_latestVersion("unknown")
{
}

ProtonModel::~ProtonModel() = default;

int ProtonModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_versions.size();
}

QVariant ProtonModel::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_versions.size()) {
        return QVariant();
    }
    const QString &version = m_versions[index.row()];
    switch (role) {
    case Qt::DisplayRole:
        return version;
    case VersionRole:
        return version;
    case IsLatestRole:
        return version == m_latestVersion;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ProtonModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "modelData";
    roles[VersionRole] = "version";
    roles[IsLatestRole] = "isLatest";
    return roles;
}

QString ProtonModel::latestVersion() const {
    return m_latestVersion;
}

QStringList ProtonModel::versions() const {
    return m_versions;
}

bool ProtonModel::isLatest(const QString &version) const {
    return version == m_latestVersion;
}

void ProtonModel::refresh() {
    beginResetModel();
    m_versions.clear();
    QStringList installed = m_protonManager.installedVersions();
    for (const auto &v : installed) {
        m_versions.append(v);
    }
    m_latestVersion = m_protonManager.latestVersion();
    endResetModel();
}

void ProtonModel::downloadVersion(const QString &version) {
    Q_EMIT m_protonManager.downloadVersion(version);
}

void ProtonModel::deleteVersion(const QString &version) {
    m_protonManager.deleteVersion(version);
}

void ProtonModel::onDownloadProgress(qint64 received, qint64 total) {
    Q_UNUSED(received);
    Q_UNUSED(total);
    // Could emit a progress signal for QML binding
}

void ProtonModel::onDownloadFinished(const QString &version) {
    Q_UNUSED(version);
    refresh();
}

void ProtonModel::onDownloadError(const QString &error) {
    qCWarning(lcProtonModel) << "Download error:" << error;
}

} // namespace Orbital
