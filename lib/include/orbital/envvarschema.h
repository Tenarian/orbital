#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>

#include "orbital/types.h"

namespace Orbital {

// Widget types that EnvVarEntry can render as in the GUI.
enum class WidgetType {
    Toggle,      // boolean → Switch
    Slider,      // number with min/max → Slider
    Multiselect, // enum string with separator → CheckBox list
    Dropdown,    // enum string (single) → ComboBox
    Text,        // string fallback → TextField
    Path,        // string (filesystem path) → TextField + Browse button
    GpuSelect,   // string → ComboBox populated by Vulkan GPU detection
    EnvGroup,    // virtual key, expands into multiple vars
    Unknown
};

// Parsed widget metadata from the x-orbital block.
struct WidgetMeta {
    WidgetType type = WidgetType::Unknown;
    QString label;
    QString description;
    QString separator = QStringLiteral(",");
    QString separatorPolicy = QStringLiteral("strict");  // "strict" or "lenient"
    QStringList lenientSeparators;
    QStringList enumValues;       // for dropdown/multiselect
    QString filter;               // for gpu-select
    QVariant defaultValue;
    double minimum = 0.0;
    double maximum = 0.0;
    bool deprecated = false;
    QString deprecationWarning;
    bool readOnly = false;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["widget"] = widgetTypeToString(type);
        obj["label"] = label;
        if (!description.isEmpty()) obj["description"] = description;
        if (!separator.isEmpty())   obj["separator"] = separator;
        if (!separatorPolicy.isEmpty()) obj["separatorPolicy"] = separatorPolicy;
        if (!lenientSeparators.isEmpty()) {
            QJsonArray arr;
            for (const auto &s : lenientSeparators) arr.append(s);
            obj["lenientSeparators"] = arr;
        }
        if (!enumValues.isEmpty()) {
            QJsonArray arr;
            for (const auto &s : enumValues) arr.append(s);
            obj["enumValues"] = arr;
        }
        if (!filter.isEmpty()) obj["filter"] = filter;
        if (!defaultValue.isNull()) obj["default"] = QJsonValue::fromVariant(defaultValue);
        if (minimum != 0.0) obj["minimum"] = minimum;
        if (maximum != 0.0) obj["maximum"] = maximum;
        if (deprecated) {
            obj["deprecated"] = true;
            if (!deprecationWarning.isEmpty()) obj["deprecationWarning"] = deprecationWarning;
        }
        if (readOnly) obj["readOnly"] = true;
        return obj;
    }

    static QString widgetTypeToString(WidgetType t) {
        switch (t) {
        case WidgetType::Toggle:      return "toggle";
        case WidgetType::Slider:      return "slider";
        case WidgetType::Multiselect: return "multiselect";
        case WidgetType::Dropdown:    return "dropdown";
        case WidgetType::Text:        return "text";
        case WidgetType::Path:        return "path";
        case WidgetType::GpuSelect:   return "gpu-select";
        case WidgetType::EnvGroup:    return "env-group";
        default:                      return "unknown";
        }
    }

    static WidgetType stringToWidgetType(const QString &s) {
        if (s == "toggle")      return WidgetType::Toggle;
        if (s == "slider")      return WidgetType::Slider;
        if (s == "multiselect") return WidgetType::Multiselect;
        if (s == "dropdown")    return WidgetType::Dropdown;
        if (s == "text")        return WidgetType::Text;
        if (s == "path")        return WidgetType::Path;
        if (s == "gpu-select")  return WidgetType::GpuSelect;
        if (s == "env-group")   return WidgetType::EnvGroup;
        return WidgetType::Unknown;
    }
};

// Parsed property definition — the full schema for one env var.
struct PropertyDef {
    QString key;
    QString jsonType;             // "boolean", "string", "number", "integer"
    WidgetMeta widget;
    QString pattern;              // regex pattern for string type
    bool locked = false;          // true → not editable in Env Vars tab
    bool isEnvGroup = false;      // true → virtual key, expands into vars
    // For env-group: ordered list of member var definitions.
    // Each member is a partial PropertyDef (only `key`, `widget`, `locked`,
    // and an optional fixed `value` are populated).
    struct GroupMember {
        QString key;
        QString fixedValue;       // if non-empty, this member has a locked value
        bool locked = false;
        WidgetMeta widget;
    };
    QList<GroupMember> groupMembers;
};

class EnvVarSchemaLoader : public QObject {
    Q_OBJECT

public:
    explicit EnvVarSchemaLoader(QObject *parent = nullptr);
    ~EnvVarSchemaLoader() override;

    // Load all schemas (built-in + user)
    void load();

    // Get all categories (x-orbital-category blocks)
    QList<QJsonObject> categories() const;

    // Get all env var entries for a category
    QList<EnvVarEntry> entriesForCategory(const QString &categoryId) const;

    // Get all entries across all categories
    QList<EnvVarEntry> allEntries() const;

    // Get entry by key
    EnvVarEntry entryForKey(const QString &key) const;

    // Get full property definition by key (includes widget metadata, enum, pattern)
    PropertyDef propertyForKey(const QString &key) const;

    // Get env-group members for a virtual key
    QList<EnvVarEntry> envGroupMembers(const QString &groupId) const;

    // Get env-group member PropertyDefs (for variable members like
    // __NV_PRIME_RENDER_OFFLOAD_PROVIDER that need gpu-select)
    QList<PropertyDef::GroupMember> envGroupMemberDefs(const QString &groupId) const;

    // Validate an env var value against its schema.
    // Returns ParseResult with .valid == false and .errorMessage set on failure.
    ParseResult validate(const QString &key, const QString &value) const;

    // Parse a value into a typed QVariant (boolean → bool, number → double,
    // multiselect → QStringList). Returns ParseResult with .parsedValue set.
    ParseResult parse(const QString &key, const QString &value) const;

    // Check if a key exists in any schema
    bool hasKey(const QString &key) const;

    // Get category for a key
    QString categoryIdForKey(const QString &key) const;

    // Get group for a key (returns empty if not part of an env-group)
    QString groupIdForKey(const QString &key) const;

    // Get all known env-group virtual keys
    QStringList envGroupIds() const;

    // Reload schemas from disk
    void reload();

signals:
    void schemaLoaded();
    void schemaError(const QString &error);

private:
    // Schema file as parsed from disk
    struct SchemaFile {
        QString path;
        QString categoryId;
        QJsonObject categoryDef;   // x-orbital-category block
        QJsonObject properties;    // properties block
    };

    QList<SchemaFile> m_schemaFiles;
    QMap<QString, EnvVarEntry> m_entries;                 // key -> entry
    QMap<QString, PropertyDef> m_properties;              // key -> full def
    QMap<QString, QList<EnvVarEntry>> m_categoryEntries;  // categoryId -> entries
    QMap<QString, QString> m_keyToCategory;               // key -> categoryId
    QMap<QString, QString> m_keyToGroup;                  // member key -> groupId
    QMap<QString, QList<EnvVarEntry>> m_envGroups;        // groupId -> member entries
    QMap<QString, QList<PropertyDef::GroupMember>> m_envGroupDefs;  // groupId -> member defs

    void loadBuiltInSchemas();
    void loadUserSchemas();
    void parseSchemaFile(const QString &path);
    void parseCategory(const SchemaFile &sf);
    void parseProperty(const QString &categoryId, const QString &key, const QJsonObject &prop);
    void parseEnvGroup(const QString &categoryId, const QString &key, const QJsonObject &orbital);
    ParseResult parseTypedValue(const PropertyDef &def, const QString &value) const;

    // Separator helpers
    QStringList splitValue(const PropertyDef &def, const QString &value) const;
};

} // namespace Orbital

Q_DECLARE_METATYPE(Orbital::WidgetType)
Q_DECLARE_METATYPE(Orbital::WidgetMeta)
Q_DECLARE_METATYPE(Orbital::PropertyDef)
