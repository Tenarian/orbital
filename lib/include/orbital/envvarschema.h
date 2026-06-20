#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

#include "orbital/types.h"

namespace Orbital {

class EnvVarSchemaLoader : public QObject {
    Q_OBJECT

public:
    explicit EnvVarSchemaLoader(QObject *parent = nullptr);
    ~EnvVarSchemaLoader() override;

    // Load all schemas (built-in + user)
    void load();

    // Get all categories
    QList<QJsonObject> categories() const;

    // Get all env var entries for a category
    QList<EnvVarEntry> entriesForCategory(const QString &categoryId) const;

    // Get all entries across all categories
    QList<EnvVarEntry> allEntries() const;

    // Get entry by key
    EnvVarEntry entryForKey(const QString &key) const;

    // Get env-group members for a virtual key
    QList<EnvVarEntry> envGroupMembers(const QString &groupId) const;

    // Validate an env var value against its schema
    ParseResult validate(const QString &key, const QString &value) const;

    // Parse a value according to its schema type
    ParseResult parse(const QString &key, const QString &value) const;

    // Check if a key exists in any schema
    bool hasKey(const QString &key) const;

    // Get category for a key
    QString categoryIdForKey(const QString &key) const;

    // Get group for a key
    QString groupIdForKey(const QString &key) const;

    // Reload schemas from disk
    void reload();

signals:
    void schemaLoaded();
    void schemaError(const QString &error);

private:
    // Schema structure
    struct SchemaFile {
        QString path;
        QString categoryId;
        QJsonObject categoryDef;
        QJsonObject properties;
    };

    QList<SchemaFile> m_schemaFiles;
    QMap<QString, EnvVarEntry> m_entries;           // key -> entry
    QMap<QString, QList<EnvVarEntry>> m_categoryEntries;  // categoryId -> entries
    QMap<QString, QString> m_keyToCategory;         // key -> categoryId
    QMap<QString, QString> m_keyToGroup;            // key -> groupId (env-group)
    QMap<QString, QList<EnvVarEntry>> m_envGroups;  // groupId -> members

    void loadBuiltInSchemas();
    void loadUserSchemas();
    void parseSchemaFile(const QString &path);
    void parseCategory(const QJsonObject &cat);
    void parseProperty(const QString &key, const QJsonObject &prop);
    void parseEnvGroup(const QString &key, const QJsonObject &group);
    EnvVarState validateValue(const QString &key, const QString &value) const;
    ParseResult parseTypedValue(const QString &key, const QJsonObject &propDef,
                                const QString &value) const;
};

} // namespace Orbital
