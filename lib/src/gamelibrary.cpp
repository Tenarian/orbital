#include "orbital/gamelibrary.h"
#include "orbital/paths.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QStandardPaths>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcGameLibrary, "orbital.gamelibrary")

namespace Orbital {

GameLibrary::GameLibrary(QObject *parent) : QAbstractListModel(parent) {}

GameLibrary::~GameLibrary() = default;

void GameLibrary::load() {
    QString path = Paths::configFile();
    QFile file(path);
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            if (!doc.isNull() && doc.isArray()) {
                m_games.clear();
                for (const auto &v : doc.array()) {
                    GameEntry entry = GameEntry::fromJson(v.toObject());
                    m_games.append(entry);
                }
            }
        }
    }
    emit layoutChanged();
}

void GameLibrary::save(const QString &id) {
    Q_UNUSED(id);
    QFile file(Paths::configFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(lcGameLibrary) << "Cannot save games:" << file.errorString();
        return;
    }
    QJsonArray arr;
    for (const auto &game : m_games) {
        arr.append(game.toJson());
    }
    file.write(QJsonDocument(arr).toJson());
    file.close();
}

void GameLibrary::updateGame(const GameEntry &entry) {
    int idx = -1;
    for (int i = 0; i < m_games.size(); ++i) {
        if (m_games[i].id == entry.id) {
            idx = i;
            break;
        }
    }
    if (idx >= 0) {
        m_games[idx] = entry;
        emit dataChanged(this->index(idx, 0), this->index(idx, 0));
        emit gameUpdated(entry.id);
    }
}

QString GameLibrary::addGame(const GameEntry &entry) {
    m_games.append(entry);
    emit layoutChanged();
    return entry.id;
}

void GameLibrary::removeGame(const QString &id) {
    int idx = -1;
    for (int i = 0; i < m_games.size(); ++i) {
        if (m_games[i].id == id) {
            idx = i;
            break;
        }
    }
    if (idx >= 0) {
        m_games.removeAt(idx);
        emit layoutChanged();
    }
}

QList<GameEntry> GameLibrary::search(const QString &query) const {
    QList<GameEntry> results;
    for (const auto &game : m_games) {
        if (game.name.toLower().contains(query.toLower())) {
            results.append(game);
        }
    }
    return results;
}

GameEntry GameLibrary::findByName(const QString &name) const {
    for (const auto &game : m_games) {
        if (game.name == name) {
            return game;
        }
    }
    return {};
}

GameEntry GameLibrary::findById(const QString &id) const {
    for (const auto &game : m_games) {
        if (game.id == id) {
            return game;
        }
    }
    return {};
}

QHash<int, QByteArray> GameLibrary::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[ExecutableRole] = "executable";
    roles[LastPlayedRole] = "lastPlayed";
    roles[PrefixModeRole] = "prefixMode";
    roles[SharedPrefixIdRole] = "sharedPrefixId";
    roles[LaunchArgsRole] = "launchArgs";
    roles[EnvVarsRole] = "envVars";
    roles[ProtonVersionRole] = "protonVersion";
    return roles;
}

int GameLibrary::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_games.size();
}

QVariant GameLibrary::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_games.size()) {
        return QVariant();
    }
    const auto &game = m_games[index.row()];
    switch (role) {
    case IdRole:
        return game.id;
    case NameRole:
        return game.name;
    case ExecutableRole:
        return game.executable;
    case LastPlayedRole:
        return game.lastPlayed;
    case PrefixModeRole:
        return game.prefixMode;
    case SharedPrefixIdRole:
        return game.sharedPrefixId;
    case LaunchArgsRole:
        return game.launchArgs;
    case EnvVarsRole:
        return QVariant::fromValue(game.envVars);
    case ProtonVersionRole:
        return game.protonVersion;
    default:
        return QVariant();
    }
}

} // namespace Orbital
