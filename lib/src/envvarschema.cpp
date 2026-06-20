#include "orbital/envvarschema.h"
#include "orbital/paths.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcEnvVarSchema, "orbital.envvarschema")

namespace Orbital {

EnvVarSchemaLoader::EnvVarSchemaLoader(QObject *parent) : QObject(parent) {}

EnvVarSchemaLoader::~EnvVarSchemaLoader() = default;

void EnvVarSchemaLoader::load() {
    loadBuiltInSchemas();
    loadUserSchemas();
    emit schemaLoaded();
}

void EnvVarSchemaLoader::reload() {
    m_schemaFiles.clear();
    m_entries.clear();
    m_categoryEntries.clear();
    m_keyToCategory.clear();
    m_keyToGroup.clear();
    m_envGroups.clear();
    load();
}

QList<QJsonObject> EnvVarSchemaLoader::categories() const {
    QList<QJsonObject> result;
    for (const auto &sf : m_schemaFiles) {
        result.append(sf.categoryDef);
    }
    return result;
}

QList<EnvVarEntry> EnvVarSchemaLoader::entriesForCategory(const QString &categoryId) const {
    return m_categoryEntries.value(categoryId);
}

QList<EnvVarEntry> EnvVarSchemaLoader::allEntries() const {
    QList<EnvVarEntry> result;
    for (const auto &entry : m_entries.values()) {
        result.append(entry);
    }
    return result;
}

EnvVarEntry EnvVarSchemaLoader::entryForKey(const QString &key) const {
    return m_entries.value(key);
}

EnvVarState EnvVarSchemaLoader::validateValue(const QString &key, const QString &value) const {
    Q_UNUSED(key);
    Q_UNUSED(value);
    return EnvVarState::Clean;
}

ParseResult EnvVarSchemaLoader::validate(const QString &key, const QString &value) const {
    ParseResult result;
    result.valid = true;
    result.parsedValue = value;
    if (!hasKey(key)) {
        result.valid = false;
        result.errorMessage = "Key not found: " + key;
    }
    return result;
}

ParseResult EnvVarSchemaLoader::parse(const QString &key, const QString &value) const {
    return validate(key, value);
}

bool EnvVarSchemaLoader::hasKey(const QString &key) const {
    return m_entries.contains(key);
}

QString EnvVarSchemaLoader::categoryIdForKey(const QString &key) const {
    return m_keyToCategory.value(key);
}

QString EnvVarSchemaLoader::groupIdForKey(const QString &key) const {
    return m_keyToGroup.value(key);
}

QList<EnvVarEntry> EnvVarSchemaLoader::envGroupMembers(const QString &groupId) const {
    return m_envGroups.value(groupId);
}

void EnvVarSchemaLoader::loadBuiltInSchemas() {
    QString schemaDir = Paths::builtinSchemas();
    QDir dir(schemaDir);
    if (!dir.exists()) {
        emit schemaError("Schema directory not found: " + schemaDir);
        return;
    }
    for (const auto &file : dir.entryInfoList(QStringList() << "*.json")) {
        parseSchemaFile(file.absoluteFilePath());
    }
}

void EnvVarSchemaLoader::loadUserSchemas() {
    QString userDir = Paths::userSchemas();
    QDir dir(userDir);
    if (!dir.exists()) {
        return;
    }
    for (const auto &file : dir.entryInfoList(QStringList() << "*.json")) {
        parseSchemaFile(file.absoluteFilePath());
    }
}

void EnvVarSchemaLoader::parseSchemaFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit schemaError("Cannot open schema file: " + path);
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (doc.isNull() || !doc.isObject()) {
        emit schemaError("Invalid JSON in: " + path);
        return;
    }
    QJsonObject obj = doc.object();
    SchemaFile sf;
    sf.path = path;
    sf.categoryDef = obj;
    if (obj.contains("id")) {
        sf.categoryId = obj["id"].toString();
    }
    if (obj.contains("properties")) {
        sf.properties = obj["properties"].toObject();
    }
    m_schemaFiles.append(sf);
    parseCategory(obj);
}

void EnvVarSchemaLoader::parseCategory(const QJsonObject &cat) {
    // Simplified: iterate properties
    if (cat.contains("properties")) {
        QJsonObject props = cat["properties"].toObject();
        for (auto it = props.constBegin(); it != props.constEnd(); ++it) {
            parseProperty(it.key(), it.value().toObject());
        }
    }
}

void EnvVarSchemaLoader::parseProperty(const QString &key, const QJsonObject &prop) {
    EnvVarEntry entry;
    entry.key = key;
    if (prop.contains("default")) {
        entry.value = prop["default"].toString();
    }
    m_entries[key] = entry;
}

void EnvVarSchemaLoader::parseEnvGroup(const QString &key, const QJsonObject &group) {
    Q_UNUSED(key);
    Q_UNUSED(group);
}

} // namespace Orbital
