#include "bridge/librarymodel.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcLibraryModel, "orbital.librarymodel")

namespace Orbital {

LibraryModel::LibraryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

LibraryModel::~LibraryModel() = default;

int LibraryModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_library.count();
}

QVariant LibraryModel::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_library.count()) {
        return QVariant();
    }
    GameEntry game = m_library.allGames().value(index.row(), {});
    switch (role) {
    case LibraryModel::IdRole:
        return game.id;
    case LibraryModel::NameRole:
        return game.name;
    case LibraryModel::ExecutableRole:
        return game.executable;
    case LibraryModel::ProtonVersionRole:
        return game.protonVersion;
    case LibraryModel::LastPlayedRole:
        return game.lastPlayed;
    case LibraryModel::PrefixModeRole:
        return game.prefixMode;
    case LibraryModel::SharedPrefixIdRole:
        return game.sharedPrefixId;
    case LibraryModel::LaunchArgsRole:
        return game.launchArgs;
    case LibraryModel::EnvVarsRole:
        return QVariant::fromValue(game.envVars);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LibraryModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[ExecutableRole] = "executable";
    roles[ProtonVersionRole] = "protonVersion";
    roles[LastPlayedRole] = "lastPlayed";
    roles[PrefixModeRole] = "prefixMode";
    roles[SharedPrefixIdRole] = "sharedPrefixId";
    roles[LaunchArgsRole] = "launchArgs";
    roles[EnvVarsRole] = "envVars";
    return roles;
}

void LibraryModel::refresh() {
    m_library.load();
    beginResetModel();
    endResetModel();
}

GameEntry LibraryModel::at(int index) const {
    if (index < 0 || index >= m_library.count()) {
        return GameEntry{};
    }
    return m_library.allGames()[index];
}

int LibraryModel::count() const {
    return m_library.count();
}

void LibraryModel::onGameAdded(const QString &id) {
    Q_UNUSED(id);
    Q_EMIT dataChanged(this->index(0, 0), this->index(this->rowCount() - 1, 0));
}

void LibraryModel::onGameRemoved(const QString &id) {
    Q_UNUSED(id);
    m_library.load();
    beginResetModel();
    endResetModel();
}

void LibraryModel::onGameUpdated(const QString &id) {
    Q_UNUSED(id);
    m_library.load();
    beginResetModel();
    endResetModel();
}

} // namespace Orbital
