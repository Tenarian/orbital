#include "orbital/gamelibrary.h"
#include "orbital/paths.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QStandardPaths>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcGameLibrary, "orbital.gamelibrary")

namespace Orbital {

// ============================================================================
// Helpers
// ============================================================================

// Sanitize a game name for use as a filename component.
// Per PRD: '/' → '∕' (U+2215 DIVISION SLASH), null bytes stripped.
// Also strip leading/trailing whitespace and other path-unsafe chars.
static QString sanitizeName(const QString &name) {
    QString out = name;
    out.replace(QLatin1Char('/'),  QChar(0x2215));  // division slash
    out.replace(QLatin1Char('\\'), QChar(0x2215));
    out.replace(QLatin1Char('\0'), QString());
    out.replace(QLatin1Char('\n'), QLatin1Char(' '));
    out.replace(QLatin1Char('\r'), QLatin1Char(' '));
    return out.trimmed();
}

// Return the short form of a UUID (first 8 chars of the canonical form,
// without braces) — used for disambiguating duplicate names.
static QString shortUuid(const QString &uuid) {
    return uuid.size() >= 8 ? uuid.left(8) : uuid;
}

// ============================================================================
// ctor / dtor
// ============================================================================

GameLibrary::GameLibrary(QObject *parent) : QAbstractListModel(parent) {}

GameLibrary::~GameLibrary() = default;

// ============================================================================
// Persistence — load() iterates the games/by-uuid/ directory; no index file.
// save(id) writes a single per-game metadata.json atomically.
// ============================================================================

void GameLibrary::load() {
    beginResetModel();
    m_games.clear();

    const QString byUuid = Paths::gamesByUuid();
    QDir dir(byUuid);
    if (!dir.exists()) {
        // No games yet — that's fine, not an error.
        endResetModel();
        return;
    }

    const auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : entries) {
        const QString metaPath = fi.absoluteFilePath() + "/metadata.json";
        QFile f(metaPath);
        if (!f.open(QIODevice::ReadOnly)) {
            qCWarning(lcGameLibrary) << "Cannot open" << metaPath << ":" << f.errorString();
            continue;
        }
        QJsonParseError parseErr{};
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseErr);
        f.close();
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            qCWarning(lcGameLibrary) << "Invalid JSON in" << metaPath << ":" << parseErr.errorString();
            continue;
        }
        GameEntry entry = GameEntry::fromJson(doc.object());
        if (entry.id.isEmpty()) {
            // Fall back to directory name if id missing.
            entry.id = fi.fileName();
        }
        m_games.append(entry);
    }

    // Sort by name for stable display.
    std::sort(m_games.begin(), m_games.end(),
              [](const GameEntry &a, const GameEntry &b) { return a.name.toLower() < b.name.toLower(); });

    endResetModel();
}

void GameLibrary::save(const QString &id) {
    if (id.isEmpty()) {
        qCWarning(lcGameLibrary) << "save(): empty id";
        return;
    }
    const GameEntry entry = findById(id);
    if (entry.id.isEmpty()) {
        qCWarning(lcGameLibrary) << "save(): game not found:" << id;
        return;
    }

    const QString dir  = Paths::gamesByUuid() + "/" + id;
    const QString path = dir + "/metadata.json";

    QDir().mkpath(dir);

    // QSaveFile takes the FINAL target path; it manages an internal .tmp
    // file and renames atomically on commit().
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(lcGameLibrary) << "Cannot open" << path << ":" << f.errorString();
        return;
    }
    const QJsonDocument doc(entry.toJson());
    f.write(doc.toJson(QJsonDocument::Indented));
    if (!f.commit()) {
        qCWarning(lcGameLibrary) << "Atomic write failed for" << path << ":" << f.errorString();
        return;
    }

    // Update the by-name/ symlink to keep it in sync.
    syncSymlink(entry);
}

// ============================================================================
// Name resolution — handles duplicate names and special-character sanitization.
// Returns a name that is safe to use as a by-name/ symlink filename and
// guaranteed unique within the current library.
// ============================================================================

QString GameLibrary::resolveName(const QString &name) const {
    const QString base = sanitizeName(name);
    if (base.isEmpty()) {
        return QString();
    }

    // First, check if no game uses this exact name.
    bool collision = false;
    for (const GameEntry &g : m_games) {
        if (sanitizeName(g.name) == base) {
            collision = true;
            break;
        }
    }
    if (!collision) {
        return base;
    }

    // Otherwise, append a short UUID segment to disambiguate.
    // Caller should supply the candidate's UUID; since resolveName is const
    // and doesn't know which game we're resolving for, we pick the next
    // unused suffix by checking the filesystem.
    QDir byName(Paths::gamesByName());
    for (int attempt = 0; attempt < 8; ++attempt) {
        const QString suffix = base + " (" + shortUuid(QUuid::createUuid().toString(QUuid::WithoutBraces)) + ")";
        if (!byName.exists(suffix)) {
            return suffix;
        }
    }
    // Last resort — append a counter.
    int n = 2;
    while (byName.exists(base + " (" + QString::number(n) + ")")) ++n;
    return base + " (" + QString::number(n) + ")";
}

// ============================================================================
// by-name/ symlink management.
//
// Symlinks are RELATIVE (../by-uuid/{uuid}/) so the data dir stays portable
// when backed up or moved. They point to the directory, not the metadata file.
// ============================================================================

void GameLibrary::syncSymlink(const GameEntry &entry) {
    if (entry.id.isEmpty() || entry.name.isEmpty()) return;

    const QString byName = Paths::gamesByName();
    QDir().mkpath(byName);

    const QString sanitized = sanitizeName(entry.name);
    const QString target = "../by-uuid/" + entry.id;  // relative
    const QString linkPath = byName + "/" + sanitized;

    // Remove existing symlink/file at linkPath if present.
    QFile::remove(linkPath);

    if (!QFile::link(target, linkPath)) {
        qCWarning(lcGameLibrary) << "Failed to create symlink" << linkPath << "->" << target;
    }
}

void GameLibrary::removeSymlink(const QString &name) {
    if (name.isEmpty()) return;
    const QString sanitized = sanitizeName(name);
    const QString linkPath = Paths::gamesByName() + "/" + sanitized;
    QFile::remove(linkPath);
}

// ============================================================================
// CRUD
// ============================================================================

QString GameLibrary::addGame(const GameEntry &entry) {
    GameEntry e = entry;

    // Auto-generate UUID if caller didn't supply one.
    if (e.id.isEmpty()) {
        e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // Default prefix mode.
    if (e.prefixMode.isEmpty()) {
        e.prefixMode = QStringLiteral("per-game");
    }

    const int row = m_games.size();
    beginInsertRows(QModelIndex(), row, row);
    m_games.append(e);
    endInsertRows();

    // Persist + create by-name symlink.
    save(e.id);

    emit gameAdded(e.id);
    return e.id;
}

void GameLibrary::removeGame(const QString &id) {
    if (id.isEmpty()) return;

    int idx = -1;
    for (int i = 0; i < m_games.size(); ++i) {
        if (m_games[i].id == id) { idx = i; break; }
    }
    if (idx < 0) {
        qCWarning(lcGameLibrary) << "removeGame(): not found:" << id;
        return;
    }

    const QString name = m_games[idx].name;

    beginRemoveRows(QModelIndex(), idx, idx);
    m_games.removeAt(idx);
    endRemoveRows();

    // Remove from disk: by-uuid/{id}/ directory + by-name/{sanitized} symlink.
    const QString gameDir = Paths::gamesByUuid() + "/" + id;
    QDir(gameDir).removeRecursively();
    removeSymlink(name);

    emit gameRemoved(id);
}

void GameLibrary::updateGame(const GameEntry &entry) {
    if (entry.id.isEmpty()) {
        qCWarning(lcGameLibrary) << "updateGame(): empty id";
        return;
    }
    int idx = -1;
    for (int i = 0; i < m_games.size(); ++i) {
        if (m_games[i].id == entry.id) { idx = i; break; }
    }
    if (idx < 0) {
        qCWarning(lcGameLibrary) << "updateGame(): not found:" << entry.id;
        return;
    }

    const QString oldName = m_games[idx].name;
    m_games[idx] = entry;

    // If the name changed, drop the old by-name symlink; save() will recreate the new one.
    if (oldName != entry.name) {
        removeSymlink(oldName);
    }

    emit dataChanged(this->index(idx, 0), this->index(idx, 0));
    emit gameUpdated(entry.id);
}

QList<GameEntry> GameLibrary::search(const QString &query) const {
    QList<GameEntry> results;
    if (query.isEmpty()) return results;
    const QString q = query.toLower();
    for (const auto &game : m_games) {
        if (game.name.toLower().contains(q)) {
            results.append(game);
        }
    }
    return results;
}

GameEntry GameLibrary::findByName(const QString &name) const {
    if (name.isEmpty()) return {};
    for (const auto &game : m_games) {
        if (game.name == name) return game;
    }
    return {};
}

GameEntry GameLibrary::findById(const QString &id) const {
    if (id.isEmpty()) return {};
    for (const auto &game : m_games) {
        if (game.id == id) return game;
    }
    return {};
}

// ============================================================================
// QAbstractListModel plumbing
// ============================================================================

QHash<int, QByteArray> GameLibrary::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole]            = "id";
    roles[NameRole]          = "name";
    roles[ExecutableRole]    = "executable";
    roles[ProtonVersionRole] = "protonVersion";
    roles[LastPlayedRole]    = "lastPlayed";
    roles[PrefixModeRole]    = "prefixMode";
    roles[SharedPrefixIdRole] = "sharedPrefixId";
    roles[LaunchArgsRole]    = "launchArgs";
    roles[EnvVarsRole]       = "envVars";
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
    case IdRole:             return game.id;
    case NameRole:           return game.name;
    case ExecutableRole:     return game.executable;
    case ProtonVersionRole:  return game.protonVersion;
    case LastPlayedRole:     return game.lastPlayed;
    case PrefixModeRole:     return game.prefixMode;
    case SharedPrefixIdRole: return game.sharedPrefixId;
    case LaunchArgsRole:     return game.launchArgs;
    case EnvVarsRole:        return QVariant::fromValue(game.envVars);
    default:                 return QVariant();
    }
}

} // namespace Orbital
