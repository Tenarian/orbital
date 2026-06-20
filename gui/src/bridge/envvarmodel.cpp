#include "bridge/envvarmodel.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcEnvVarModel, "orbital.envvarmodel")

namespace Orbital {

EnvVarModel::EnvVarModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

EnvVarModel::~EnvVarModel() = default;

int EnvVarModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_entries.size();
}

QVariant EnvVarModel::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_entries.size()) {
        return QVariant();
    }
    const EnvVarEntry &entry = m_entries[index.row()];
    switch (role) {
    case KeyRole:
        return entry.key;
    case ValueRole:
        return entry.value;
    case SourceRole:
        return entry.source == EnvVarSource::Generated ? "generated" : "custom";
    case StateRole:
        return entry.state == EnvVarState::Clean ? "clean"
               : entry.state == EnvVarState::Modified ? "modified" : "invalid";
    case CategoryRole:
        return entry.categoryId;
    case GroupRole:
        return entry.groupId;
    case LockedRole:
        return entry.locked;
    case ErrorRole:
        return entry.errorMessage;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> EnvVarModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "key";
    roles[ValueRole] = "value";
    roles[SourceRole] = "source";
    roles[StateRole] = "state";
    roles[CategoryRole] = "categoryId";
    roles[GroupRole] = "groupId";
    roles[LockedRole] = "locked";
    roles[ErrorRole] = "errorMessage";
    return roles;
}

void EnvVarModel::load() {
    m_loader.load();
    m_entries = m_loader.allEntries();
    // Merge custom vars
    for (auto it = m_customVars.constBegin(); it != m_customVars.constEnd(); ++it) {
        EnvVarEntry entry;
        entry.key = it.key();
        entry.value = it.value();
        entry.source = EnvVarSource::Custom;
        entry.state = EnvVarState::Clean;
        m_entries.append(entry);
    }
    beginResetModel();
    endResetModel();
}

void EnvVarModel::reload() {
    load();
}

void EnvVarModel::setValue(const QString &key, const QString &value) {
    for (auto &entry : m_entries) {
        if (entry.key == key) {
            entry.value = value;
            entry.state = EnvVarState::Modified;
            break;
        }
    }
    m_customVars[key] = value;
    Q_EMIT dataChanged(this->index(0, 0), this->index(rowCount() - 1, 0));
}

void EnvVarModel::addCustom(const QString &key, const QString &value) {
    m_customVars[key] = value;
    EnvVarEntry entry;
    entry.key = key;
    entry.value = value;
    entry.source = EnvVarSource::Custom;
    entry.state = EnvVarState::Clean;
    m_entries.append(entry);
    beginResetModel();
    endResetModel();
}

void EnvVarModel::removeCustom(const QString &key) {
    m_customVars.remove(key);
    m_entries.removeAll(EnvVarEntry{});
    // Rebuild entries without the removed key
    QList<EnvVarEntry> filtered;
    for (const auto &e : m_entries) {
        if (e.key != key) {
            filtered.append(e);
        }
    }
    m_entries = filtered;
    beginResetModel();
    endResetModel();
}

QList<EnvVarEntry> EnvVarModel::entries() const {
    return m_entries;
}

QList<EnvVarEntry> EnvVarModel::entriesForCategory(const QString &categoryId) const {
    return m_loader.entriesForCategory(categoryId);
}

QList<EnvVarEntry> EnvVarModel::allEntries() const {
    return m_entries;
}

bool EnvVarModel::hasKey(const QString &key) const {
    for (const auto &e : m_entries) {
        if (e.key == key) return true;
    }
    return m_customVars.contains(key);
}

QString EnvVarModel::categoryIdForKey(const QString &key) const {
    return m_loader.categoryIdForKey(key);
}

int EnvVarModel::count() const {
    return m_entries.size();
}

void EnvVarModel::onSchemaLoaded() {
    Q_EMIT m_loader.schemaLoaded();
}

void EnvVarModel::onSchemaError(const QString &error) {
    qCWarning(lcEnvVarModel) << "Schema error:" << error;
}

} // namespace Orbital
