#include "orbital/envvarschema.h"
#include "orbital/paths.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QVariant>

Q_LOGGING_CATEGORY(lcEnvVarSchema, "orbital.envvarschema")

namespace Orbital {

// ============================================================================
// ctor / dtor
// ============================================================================

EnvVarSchemaLoader::EnvVarSchemaLoader(QObject *parent) : QObject(parent) {}
EnvVarSchemaLoader::~EnvVarSchemaLoader() = default;

// ============================================================================
// load() — read all built-in + user schemas from disk
// ============================================================================

void EnvVarSchemaLoader::load() {
    loadBuiltInSchemas();
    loadUserSchemas();
    emit schemaLoaded();
}

void EnvVarSchemaLoader::reload() {
    m_schemaFiles.clear();
    m_entries.clear();
    m_properties.clear();
    m_categoryEntries.clear();
    m_keyToCategory.clear();
    m_keyToGroup.clear();
    m_envGroups.clear();
    m_envGroupDefs.clear();
    load();
}

// ============================================================================
// Built-in schemas — assets/schema/*.json
// User schemas — ~/.config/orbital/schema/*.json (overrides built-ins by key)
// ============================================================================

void EnvVarSchemaLoader::loadBuiltInSchemas() {
    const QString schemaDir = Paths::builtinSchemas();
    QDir dir(schemaDir);
    if (!dir.exists()) {
        emit schemaError("Schema directory not found: " + schemaDir);
        return;
    }
    const auto files = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
    for (const QFileInfo &fi : files) {
        // launch-filter.json is not an env-var schema — skip it.
        if (fi.fileName() == QLatin1String("launch-filter.json")) continue;
        parseSchemaFile(fi.absoluteFilePath());
    }
}

void EnvVarSchemaLoader::loadUserSchemas() {
    const QString userDir = Paths::userSchemas();
    QDir dir(userDir);
    if (!dir.exists()) return;
    const auto files = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
    for (const QFileInfo &fi : files) {
        parseSchemaFile(fi.absoluteFilePath());
    }
}

void EnvVarSchemaLoader::parseSchemaFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit schemaError("Cannot open schema file: " + path);
        return;
    }
    QJsonParseError parseErr{};
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    file.close();
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit schemaError("Invalid JSON in " + path + ": " + parseErr.errorString());
        return;
    }

    QJsonObject obj = doc.object();
    SchemaFile sf;
    sf.path = path;

    // Read category metadata from x-orbital-category block.
    const QJsonObject cat = obj.value("x-orbital-category").toObject();
    sf.categoryDef = cat;
    sf.categoryId = cat.value("id").toString();
    if (sf.categoryId.isEmpty()) {
        // Fall back to filename without extension.
        sf.categoryId = QFileInfo(path).completeBaseName();
    }

    if (obj.contains("properties")) {
        sf.properties = obj.value("properties").toObject();
    }

    m_schemaFiles.append(sf);
    parseCategory(sf);
}

void EnvVarSchemaLoader::parseCategory(const SchemaFile &sf) {
    const QJsonObject props = sf.properties;
    for (auto it = props.constBegin(); it != props.constEnd(); ++it) {
        const QString key = it.key();
        const QJsonObject prop = it.value().toObject();
        parseProperty(sf.categoryId, key, prop);
    }
}

// ============================================================================
// parseProperty — read a single property definition into PropertyDef + EnvVarEntry
// ============================================================================

void EnvVarSchemaLoader::parseProperty(const QString &categoryId,
                                        const QString &key,
                                        const QJsonObject &prop) {
    const QJsonObject orbital = prop.value("x-orbital").toObject();

    // Check if this is an env-group (virtual key that expands into multiple vars).
    const QString typeField = orbital.value("type").toString();
    if (typeField == QLatin1String("env-group") ||
        orbital.contains("vars")) {
        parseEnvGroup(categoryId, key, orbital);
        return;
    }

    PropertyDef def;
    def.key = key;
    def.jsonType = prop.value("type").toString();
    def.pattern = prop.value("pattern").toString();
    def.locked = prop.value("locked").toBool(false) ||
                 orbital.value("readOnly").toBool(false);

    // Widget metadata
    def.widget.label = orbital.value("label").toString();
    def.widget.description = orbital.value("description").toString();
    def.widget.filter = orbital.value("filter").toString();
    def.widget.deprecated = orbital.value("deprecated").toBool(false);
    def.widget.deprecationWarning = orbital.value("deprecationWarning").toString();
    def.widget.readOnly = orbital.value("readOnly").toBool(false);

    const QString widgetStr = orbital.value("widget").toString();
    def.widget.type = WidgetMeta::stringToWidgetType(widgetStr);

    // If no widget specified, infer from type/enum.
    if (def.widget.type == WidgetType::Unknown) {
        if (prop.contains("enum")) {
            def.widget.type = WidgetType::Dropdown;
        } else if (def.jsonType == "boolean") {
            def.widget.type = WidgetType::Toggle;
        } else if (def.jsonType == "number" || def.jsonType == "integer") {
            def.widget.type = WidgetType::Slider;
        } else {
            def.widget.type = WidgetType::Text;
        }
    }

    // Enum values
    if (prop.contains("enum")) {
        const QJsonArray arr = prop.value("enum").toArray();
        for (const auto &v : arr) def.widget.enumValues.append(v.toString());
    }

    // Multiselect separator config
    def.widget.separator = orbital.value("separator").toString(QStringLiteral(","));
    def.widget.separatorPolicy = orbital.value("separator-policy").toString(QStringLiteral("strict"));
    if (orbital.contains("lenient-separators")) {
        const QJsonArray arr = orbital.value("lenient-separators").toArray();
        for (const auto &v : arr) def.widget.lenientSeparators.append(v.toString());
    }

    // Min/max for sliders
    if (prop.contains("minimum")) def.widget.minimum = prop.value("minimum").toDouble(0.0);
    if (prop.contains("maximum")) def.widget.maximum = prop.value("maximum").toDouble(0.0);

    // Default value
    if (prop.contains("default")) {
        def.widget.defaultValue = prop.value("default").toVariant();
    }

    // Build the EnvVarEntry summary
    EnvVarEntry entry;
    entry.key = key;
    entry.source = EnvVarSource::Generated;
    entry.state = EnvVarState::Clean;
    entry.categoryId = categoryId;
    entry.locked = def.locked;
    if (def.widget.defaultValue.isValid()) {
        // Render the default as a string for the entry value.
        const QVariant &dv = def.widget.defaultValue;
        if (dv.typeId() == QMetaType::Bool) {
            entry.value = dv.toBool() ? QStringLiteral("1") : QStringLiteral("0");
        } else {
            entry.value = dv.toString();
        }
    }

    m_properties.insert(key, def);
    m_entries.insert(key, entry);
    m_keyToCategory.insert(key, categoryId);
    m_categoryEntries[categoryId].append(entry);
}

// ============================================================================
// parseEnvGroup — handle virtual keys like PRIME_OFFLOAD / DXVK_NVAPI
//
// Per PRD §env-group:
//   - The virtual key is never injected into the game env; it only represents
//     the group in the schema.
//   - When toggled ON → expands into all vars listed under `vars`.
//   - Vars may have a fixed `value` (locked) or their own widget (gpu-select).
// ============================================================================

void EnvVarSchemaLoader::parseEnvGroup(const QString &categoryId,
                                        const QString &key,
                                        const QJsonObject &orbital) {
    PropertyDef def;
    def.key = key;
    def.isEnvGroup = true;
    def.widget.type = WidgetType::EnvGroup;
    def.widget.label = orbital.value("label").toString();
    def.widget.description = orbital.value("description").toString();

    const QJsonObject vars = orbital.value("vars").toObject();
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
        const QString memberKey = it.key();
        const QJsonObject memberObj = it.value().toObject();

        PropertyDef::GroupMember member;
        member.key = memberKey;
        member.locked = memberObj.value("locked").toBool(false);
        member.fixedValue = memberObj.value("value").toString();

        // Member widget (only relevant if not locked)
        const QJsonObject memberOrbital = memberObj;  // for env-group members,
                                                       // metadata is at top level
        const QString widgetStr = memberOrbital.value("widget").toString();
        member.widget.type = WidgetMeta::stringToWidgetType(widgetStr);
        member.widget.filter = memberOrbital.value("filter").toString();
        member.widget.label = memberOrbital.value("label").toString(memberKey);

        if (memberObj.contains("enum")) {
            const QJsonArray arr = memberObj.value("enum").toArray();
            for (const auto &v : arr) member.widget.enumValues.append(v.toString());
        }

        def.groupMembers.append(member);

        // Build an EnvVarEntry for the member too.
        EnvVarEntry memberEntry;
        memberEntry.key = memberKey;
        memberEntry.source = EnvVarSource::Generated;
        memberEntry.state = EnvVarState::Clean;
        memberEntry.categoryId = categoryId;
        memberEntry.groupId = key;
        memberEntry.locked = member.locked;
        if (!member.fixedValue.isEmpty()) {
            memberEntry.value = member.fixedValue;
        }
        m_entries.insert(memberKey, memberEntry);
        m_keyToCategory.insert(memberKey, categoryId);
        m_keyToGroup.insert(memberKey, key);
        m_envGroups[key].append(memberEntry);
    }

    // Store the virtual key itself as an entry too (so hasKey() works for it).
    EnvVarEntry groupEntry;
    groupEntry.key = key;
    groupEntry.source = EnvVarSource::Generated;
    groupEntry.state = EnvVarState::Clean;
    groupEntry.categoryId = categoryId;
    groupEntry.groupId = key;  // self-reference: this is the group virtual key
    m_entries.insert(key, groupEntry);
    m_keyToCategory.insert(key, categoryId);
    m_categoryEntries[categoryId].append(groupEntry);

    m_properties.insert(key, def);
    m_envGroupDefs.insert(key, def.groupMembers);
}

// ============================================================================
// Accessors
// ============================================================================

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
    return m_entries.values();
}

EnvVarEntry EnvVarSchemaLoader::entryForKey(const QString &key) const {
    return m_entries.value(key);
}

PropertyDef EnvVarSchemaLoader::propertyForKey(const QString &key) const {
    return m_properties.value(key);
}

QList<EnvVarEntry> EnvVarSchemaLoader::envGroupMembers(const QString &groupId) const {
    return m_envGroups.value(groupId);
}

QList<PropertyDef::GroupMember> EnvVarSchemaLoader::envGroupMemberDefs(const QString &groupId) const {
    return m_envGroupDefs.value(groupId);
}

QStringList EnvVarSchemaLoader::envGroupIds() const {
    return m_envGroupDefs.keys();
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

// ============================================================================
// splitValue — split a multiselect value into individual tokens, honoring
// the separator policy (strict = single canonical separator; lenient =
// accept any of the listed separators).
// ============================================================================

QStringList EnvVarSchemaLoader::splitValue(const PropertyDef &def, const QString &value) const {
    if (value.isEmpty()) return {};

    if (def.widget.separatorPolicy == QLatin1String("lenient") &&
        !def.widget.lenientSeparators.isEmpty()) {
        // Build a regex that matches any of the lenient separators.
        QString pattern;
        for (const QString &sep : def.widget.lenientSeparators) {
            if (!pattern.isEmpty()) pattern += "|";
            pattern += QRegularExpression::escape(sep);
        }
        QRegularExpression re(pattern);
        QStringList parts = value.split(re, Qt::SkipEmptyParts);
        // Trim whitespace from each part (handles ", " lenient separator).
        for (auto &p : parts) p = p.trimmed();
        return parts;
    }

    return value.split(def.widget.separator, Qt::SkipEmptyParts);
}

// ============================================================================
// parseTypedValue — convert a raw string value into a typed QVariant.
//
// For boolean: "1"/"true"/"yes" → true, "0"/"false"/"no" → false.
// For number/integer: parse as double, validate min/max.
// For multiselect: split on separator → QStringList.
// For dropdown/text/path: validate enum/pattern if present.
// For env-group virtual keys: always invalid (they are not real env vars).
// ============================================================================

ParseResult EnvVarSchemaLoader::parseTypedValue(const PropertyDef &def, const QString &value) const {
    ParseResult result;

    if (def.isEnvGroup) {
        result.valid = false;
        result.errorMessage = QStringLiteral("'%1' is an env-group virtual key and cannot be set directly").arg(def.key);
        return result;
    }

    const QString t = def.jsonType;

    if (t == QLatin1String("boolean")) {
        const QString lower = value.trimmed().toLower();
        if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") {
            result.valid = true;
            result.parsedValue = true;
        } else if (lower == "0" || lower == "false" || lower == "no" || lower == "off" || lower.isEmpty()) {
            result.valid = true;
            result.parsedValue = false;
        } else {
            result.valid = false;
            result.errorMessage = QStringLiteral("Expected boolean (1/0, true/false, yes/no, on/off), got '%1'").arg(value);
        }
        return result;
    }

    if (t == QLatin1String("number") || t == QLatin1String("integer")) {
        bool ok = false;
        double n = value.toDouble(&ok);
        if (!ok) {
            result.valid = false;
            result.errorMessage = QStringLiteral("Expected %1, got '%2'").arg(t, value);
            return result;
        }
        if (def.widget.minimum != 0.0 || def.widget.maximum != 0.0) {
            if (n < def.widget.minimum) {
                result.valid = false;
                result.errorMessage = QStringLiteral("Value %1 is below minimum %2")
                                          .arg(value).arg(def.widget.minimum);
                return result;
            }
            if (def.widget.maximum != 0.0 && n > def.widget.maximum) {
                result.valid = false;
                result.errorMessage = QStringLiteral("Value %1 is above maximum %2")
                                          .arg(value).arg(def.widget.maximum);
                return result;
            }
        }
        result.valid = true;
        if (t == "integer") result.parsedValue = static_cast<qlonglong>(n);
        else result.parsedValue = n;
        return result;
    }

    if (def.widget.type == WidgetType::Multiselect) {
        const QStringList parts = splitValue(def, value);
        if (!def.widget.enumValues.isEmpty()) {
            for (const QString &p : parts) {
                if (!def.widget.enumValues.contains(p)) {
                    result.valid = false;
                    result.errorMessage = QStringLiteral("'%1' is not a valid value. Allowed: %2")
                                              .arg(p, def.widget.enumValues.join(", "));
                    return result;
                }
            }
        }
        result.valid = true;
        result.parsedValue = parts;
        return result;
    }

    // dropdown / text / path / gpu-select / unknown → string with optional validation
    if (!def.widget.enumValues.isEmpty()) {
        if (!def.widget.enumValues.contains(value)) {
            result.valid = false;
            result.errorMessage = QStringLiteral("'%1' is not one of: %2")
                                      .arg(value, def.widget.enumValues.join(", "));
            return result;
        }
    }

    if (!def.pattern.isEmpty()) {
        QRegularExpression re(def.pattern);
        if (!re.match(value).hasMatch()) {
            result.valid = false;
            result.errorMessage = QStringLiteral("'%1' does not match required pattern: %2")
                                      .arg(value, def.pattern);
            return result;
        }
    }

    result.valid = true;
    result.parsedValue = value;
    return result;
}

// ============================================================================
// validate / parse
// ============================================================================

ParseResult EnvVarSchemaLoader::validate(const QString &key, const QString &value) const {
    ParseResult result;
    if (!m_properties.contains(key)) {
        // Unknown key — allow it (user may add custom vars). Mark valid.
        result.valid = true;
        result.parsedValue = value;
        return result;
    }
    return parseTypedValue(m_properties.value(key), value);
}

ParseResult EnvVarSchemaLoader::parse(const QString &key, const QString &value) const {
    return validate(key, value);
}

} // namespace Orbital
