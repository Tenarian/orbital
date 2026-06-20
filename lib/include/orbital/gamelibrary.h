#pragma once

#include <QAbstractListModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QString>
#include <QStringList>

#include "orbital/paths.h"
#include "orbital/types.h"

namespace Orbital {

class GameLibrary : public QAbstractListModel {
    Q_OBJECT

public:
    explicit GameLibrary(QObject *parent = nullptr);
    ~GameLibrary() override;

    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ExecutableRole,
        ProtonVersionRole,
        LastPlayedRole,
        PrefixModeRole,
        SharedPrefixIdRole,
        LaunchArgsRole,
        EnvVarsRole
    };

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // CRUD operations
    QString addGame(const GameEntry &entry);        // returns new UUID
    void removeGame(const QString &id);
    void updateGame(const GameEntry &entry);
    GameEntry findById(const QString &id) const;
    GameEntry findByName(const QString &name) const;
    QList<GameEntry> search(const QString &query) const;

    // Persistence
    void load();
    void save(const QString &id);

    // Additional accessors
    QList<GameEntry> allGames() const { return m_games; }
    GameEntry first() const { return m_games.isEmpty() ? GameEntry{} : m_games.first(); }
    int count() const { return m_games.size(); }
    bool isEmpty() const { return m_games.isEmpty(); }

signals:
    void gameAdded(const QString &id);
    void gameRemoved(const QString &id);
    void gameUpdated(const QString &id);

private:
    QList<GameEntry> m_games;

    // Internal helpers
    void syncSymlink(const GameEntry &entry);
    void removeSymlink(const QString &name);
    QString resolveName(const QString &name) const;
};

} // namespace Orbital
