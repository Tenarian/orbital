#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QList>

#include "orbital/gamelibrary.h"
#include "orbital/paths.h"
#include "orbital/types.h"

namespace Orbital {

class LibraryModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit LibraryModel(QObject *parent = nullptr);
    ~LibraryModel() override;

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

    // Expose GameLibrary methods
    QString addGame(const QString &name, const QString &executable);
    void removeGame(const QString &id);
    void updateGame(const QString &id, const QString &name = {},
                    const QString &executable = {}, const QString &protonVersion = {},
                    const QString &launchArgs = {});
    GameEntry gameById(const QString &id) const;
    QList<GameEntry> allGames() const;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // QML-friendly accessors
    Q_INVOKABLE void refresh();
    Q_INVOKABLE GameEntry at(int index) const;
    Q_INVOKABLE int count() const;

public slots:
    void onGameAdded(const QString &id);
    void onGameRemoved(const QString &id);
    void onGameUpdated(const QString &id);

private:
    GameLibrary m_library;
};

} // namespace Orbital
