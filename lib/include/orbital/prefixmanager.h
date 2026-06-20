#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>

#include "orbital/paths.h"
#include "orbital/types.h"

namespace Orbital {

class PrefixManager : public QObject {
    Q_OBJECT

public:
    explicit PrefixManager(QObject *parent = nullptr);
    ~PrefixManager() override;

    // Get prefix path for a game
    QString prefixPath(const QString &gameId) const;

    // Get or create prefix mode for a game
    QString getPrefixMode(const QString &gameId) const;
    void setPrefixMode(const QString &gameId, const QString &mode);

    // Shared prefix operations
    QList<PrefixMeta> listSharedPrefixes() const;
    QString createSharedPrefix(const PrefixMeta &meta);
    void deleteSharedPrefix(const QString &id);
    void assignGameToSharedPrefix(const QString &gameId, const QString &prefixId);

    // Reset prefix (delete and recreate)
    void resetPrefix(const QString &gameId);

    // Run a command inside the prefix
    bool runInPrefix(const QString &prefixPath, const QString &command,
                     const QStringList &args = {}) const;

    // Open prefix in file manager
    void openInFileManager(const QString &prefixPath) const;

signals:
    void prefixCreated(const QString &path);
    void prefixReset(const QString &gameId);
    void prefixError(const QString &error);

private:
    void ensurePrefixExists(const QString &path) const;
    PrefixMeta loadSharedMeta(const QString &id) const;
    void saveSharedMeta(const QString &id, const PrefixMeta &meta) const;
};

} // namespace Orbital
