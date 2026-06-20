#include <QObject>
#include <QTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "orbital/envvarschema.h"
#include "orbital/paths.h"
#include "test_envvarschema.h"

namespace Orbital {

void TestEnvVarSchema::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
}

void TestEnvVarSchema::cleanup() {
    // Clear user schema dir between tests so createTestSchema state doesn't leak.
    const QString userDir = Paths::userSchemas();
    QDir d(userDir);
    if (d.exists()) d.removeRecursively();
}

// Helper: write a JSON object to the user schema dir.
void TestEnvVarSchema::createTestSchema(const QString &filename, const QJsonObject &schema) {
    const QString userDir = Paths::userSchemas();
    QDir().mkpath(userDir);
    QFile f(userDir + "/" + filename);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(schema).toJson());
    f.close();
}

// ============================================================================
// loadBuiltIn — verify all 7 built-in schema files load with categories
// ============================================================================

void TestEnvVarSchema::testCase_loadBuiltIn() {
    EnvVarSchemaLoader loader;
    loader.load();

    const auto cats = loader.categories();
    QVERIFY(cats.size() >= 7);  // proton, vulkan, mesa, radeon, nvidia, mangohud, wine

    QStringList catIds;
    for (const QJsonObject &c : cats) catIds.append(c.value("id").toString());
    QVERIFY(catIds.contains("proton"));
    QVERIFY(catIds.contains("vulkan"));
    QVERIFY(catIds.contains("nvidia"));
}

// ============================================================================
// loadUserSchema — write a custom schema to the user dir, verify it loads
// ============================================================================

void TestEnvVarSchema::testCase_loadUserSchema() {
    QJsonObject schema;
    QJsonObject cat;
    cat["id"] = "custom-cat";
    cat["label"] = "Custom";
    schema["x-orbital-category"] = cat;

    QJsonObject props;
    QJsonObject prop;
    prop["type"] = "string";
    QJsonObject orbital;
    orbital["label"] = "My Custom Var";
    orbital["widget"] = "text";
    prop["x-orbital"] = orbital;
    props["MY_CUSTOM_VAR"] = prop;
    schema["properties"] = props;

    createTestSchema("custom.json", schema);

    EnvVarSchemaLoader loader;
    loader.load();

    QVERIFY(loader.hasKey("MY_CUSTOM_VAR"));
    auto entry = loader.entryForKey("MY_CUSTOM_VAR");
    QCOMPARE(entry.categoryId, QString("custom-cat"));

    auto def = loader.propertyForKey("MY_CUSTOM_VAR");
    QCOMPARE(def.widget.type, WidgetType::Text);
    QCOMPARE(def.widget.label, QString("My Custom Var"));
}

// ============================================================================
// entriesForCategory — vulkan category should have DXVK_HUD, DXVK_LOG_LEVEL, etc.
// ============================================================================

void TestEnvVarSchema::testCase_entriesForCategory() {
    EnvVarSchemaLoader loader;
    loader.load();

    const auto entries = loader.entriesForCategory("vulkan");
    QVERIFY(entries.size() > 10);

    QStringList keys;
    for (const EnvVarEntry &e : entries) keys.append(e.key);
    QVERIFY(keys.contains("DXVK_HUD"));
    QVERIFY(keys.contains("DXVK_LOG_LEVEL"));
    QVERIFY(keys.contains("VK_LOADER_DEBUG"));
}

// ============================================================================
// entryForKey — verify widget metadata parsed correctly
// ============================================================================

void TestEnvVarSchema::testCase_entryForKey() {
    EnvVarSchemaLoader loader;
    loader.load();

    QVERIFY(loader.hasKey("PROTON_ENABLE_WAYLAND"));
    QVERIFY(loader.hasKey("DXVK_HUD"));

    auto def = loader.propertyForKey("PROTON_ENABLE_WAYLAND");
    QCOMPARE(def.jsonType, QString("boolean"));
    QCOMPARE(def.widget.type, WidgetType::Toggle);
    QCOMPARE(def.widget.label, QString("Enable Wayland"));

    auto hudDef = loader.propertyForKey("DXVK_HUD");
    QCOMPARE(hudDef.widget.type, WidgetType::Multiselect);
    QCOMPARE(hudDef.widget.separator, QString(","));
    QCOMPARE(hudDef.widget.separatorPolicy, QString("lenient"));
    QVERIFY(hudDef.widget.lenientSeparators.size() >= 3);
    QVERIFY(hudDef.widget.enumValues.contains("fps"));
    QVERIFY(hudDef.widget.enumValues.contains("frametimes"));
}

// ============================================================================
// validateValue — boolean, number with min/max, enum, multiselect, pattern
// ============================================================================

void TestEnvVarSchema::testCase_validateValue() {
    EnvVarSchemaLoader loader;
    loader.load();

    // Boolean: "1" valid, "yes" valid, "maybe" invalid
    QVERIFY(loader.validate("PROTON_ENABLE_WAYLAND", "1").valid);
    QVERIFY(loader.validate("PROTON_ENABLE_WAYLAND", "yes").valid);
    QVERIFY(!loader.validate("PROTON_ENABLE_WAYLAND", "maybe").valid);

    // Number with min/max: DXVK_FRAME_RATE [1,240]
    QVERIFY(loader.validate("DXVK_FRAME_RATE", "60").valid);
    QVERIFY(!loader.validate("DXVK_FRAME_RATE", "0").valid);      // below min
    QVERIFY(!loader.validate("DXVK_FRAME_RATE", "300").valid);    // above max
    QVERIFY(!loader.validate("DXVK_FRAME_RATE", "abc").valid);    // not a number

    // Enum: DXVK_LOG_LEVEL { none, info, debug, verbose }
    QVERIFY(loader.validate("DXVK_LOG_LEVEL", "info").valid);
    QVERIFY(!loader.validate("DXVK_LOG_LEVEL", "traceback").valid);

    // Multiselect with lenient separators: DXVK_HUD accepts "fps,frametimes" and "fps:frametimes"
    QVERIFY(loader.validate("DXVK_HUD", "fps,frametimes").valid);
    QVERIFY(loader.validate("DXVK_HUD", "fps:frametimes").valid);  // lenient
    QVERIFY(loader.validate("DXVK_HUD", "fps|frametimes").valid);  // lenient
    QVERIFY(!loader.validate("DXVK_HUD", "fps,bogus").valid);      // bogus value

    // Pattern: VK_LOADER_DEVICE_SELECT matches ^0x[0-9a-f]{4}:0x[0-9a-f]{4}$|^[0-9]+:[0-9]+$
    QVERIFY(loader.validate("VK_LOADER_DEVICE_SELECT", "0x10de:0x1b80").valid);
    QVERIFY(loader.validate("VK_LOADER_DEVICE_SELECT", "1:2").valid);
    QVERIFY(!loader.validate("VK_LOADER_DEVICE_SELECT", "not-a-pci-id").valid);

    // Unknown key — should be allowed (user can add custom vars)
    QVERIFY(loader.validate("TOTALLY_UNKNOWN_VAR", "anything").valid);
}

// ============================================================================
// parseValue — verify typed QVariant output
// ============================================================================

void TestEnvVarSchema::testCase_parseValue() {
    EnvVarSchemaLoader loader;
    loader.load();

    auto b = loader.parse("PROTON_ENABLE_WAYLAND", "1");
    QVERIFY(b.valid);
    QCOMPARE(b.parsedValue.toBool(), true);

    auto n = loader.parse("DXVK_FRAME_RATE", "120");
    QVERIFY(n.valid);
    QCOMPARE(n.parsedValue.toDouble(), 120.0);

    auto ms = loader.parse("DXVK_HUD", "fps,frametimes");
    QVERIFY(ms.valid);
    auto list = ms.parsedValue.toStringList();
    QCOMPARE(list.size(), 2);
    QVERIFY(list.contains("fps"));
    QVERIFY(list.contains("frametimes"));
}

// ============================================================================
// envGroup — nvidia.json PRIME_OFFLOAD should parse as an env-group with
// locked members + a gpu-select member
// ============================================================================

void TestEnvVarSchema::testCase_envGroup() {
    EnvVarSchemaLoader loader;
    loader.load();

    const QStringList groups = loader.envGroupIds();
    QVERIFY(groups.contains("PRIME_OFFLOAD"));
    QVERIFY(groups.contains("DXVK_NVAPI"));

    const auto members = loader.envGroupMembers("PRIME_OFFLOAD");
    QVERIFY(members.size() >= 3);  // __NV_PRIME_RENDER_OFFLOAD, __NV_PRIME_RENDER_OFFLOAD_PROVIDER, __GLX_VENDOR_LIBRARY_NAME

    const auto defs = loader.envGroupMemberDefs("PRIME_OFFLOAD");
    QVERIFY(defs.size() >= 3);

    // Verify __NV_PRIME_RENDER_OFFLOAD is locked with value "1"
    bool foundOffload = false;
    bool foundProvider = false;
    for (const PropertyDef::GroupMember &m : defs) {
        if (m.key == "__NV_PRIME_RENDER_OFFLOAD") {
            foundOffload = true;
            QVERIFY(m.locked);
            QCOMPARE(m.fixedValue, QString("1"));
        }
        if (m.key == "__NV_PRIME_RENDER_OFFLOAD_PROVIDER") {
            foundProvider = true;
            QVERIFY(!m.locked);
            QCOMPARE(m.widget.type, WidgetType::GpuSelect);
            QCOMPARE(m.widget.filter, QString("nvidia"));
        }
    }
    QVERIFY(foundOffload);
    QVERIFY(foundProvider);

    // Virtual key itself should be in entries but its value can't be set.
    QVERIFY(loader.hasKey("PRIME_OFFLOAD"));
    auto r = loader.validate("PRIME_OFFLOAD", "1");
    QVERIFY(!r.valid);  // env-group virtual keys can't be set directly
}

// ============================================================================
// reload — modify schema on disk, reload, verify change
// ============================================================================

void TestEnvVarSchema::testCase_reload() {
    EnvVarSchemaLoader loader;
    loader.load();
    QVERIFY(!loader.hasKey("RELOAD_TEST_VAR"));

    QJsonObject schema;
    QJsonObject cat; cat["id"] = "reload-test"; schema["x-orbital-category"] = cat;
    QJsonObject props, prop, orbital;
    orbital["widget"] = "text"; orbital["label"] = "Reload Test";
    prop["x-orbital"] = orbital;
    props["RELOAD_TEST_VAR"] = prop;
    schema["properties"] = props;
    createTestSchema("reload.json", schema);

    loader.reload();
    QVERIFY(loader.hasKey("RELOAD_TEST_VAR"));
}

// ============================================================================
// invalidSchema — corrupt JSON file should emit schemaError, not crash
// ============================================================================

void TestEnvVarSchema::testCase_invalidSchema() {
    const QString userDir = Paths::userSchemas();
    QDir().mkpath(userDir);
    QFile f(userDir + "/broken.json");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("{ this is not valid json ]]]}");
    f.close();

    EnvVarSchemaLoader loader;
    QSignalSpy spy(&loader, &EnvVarSchemaLoader::schemaError);
    loader.load();
    QVERIFY(spy.count() >= 1);
}

} // namespace Orbital

int main(int argc, char *argv[]) {
    Orbital::TestEnvVarSchema test;
    return QTest::qExec(&test, argc, argv);
}
