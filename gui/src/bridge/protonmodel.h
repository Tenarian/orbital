#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QList>

#include "orbital/protonmanager.h"
#include "orbital/types.h"

namespace Orbital {

class ProtonModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        VersionRole = Qt::UserRole + 1,
        IsLatestRole
    };

    explicit ProtonModel(QObject *parent = nullptr);
    ~ProtonModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString latestVersion() const;
    Q_INVOKABLE void downloadVersion(const QString &version);
    Q_INVOKABLE void deleteVersion(const QString &version);

    // QML-friendly accessors
    Q_INVOKABLE QStringList versions() const;
    Q_INVOKABLE bool isLatest(const QString &version) const;

public slots:
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(const QString &version);
    void onDownloadError(const QString &error);

private:
    ProtonManager m_protonManager;
    QStringList m_versions;
    QString m_latestVersion;
};

} // namespace Orbital
