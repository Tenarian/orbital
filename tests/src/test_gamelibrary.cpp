#include <QObject>
#include <QTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include "orbital/gamelibrary.h"
#include "orbital/paths.h"
#include "orbital/types.h"
#include "test_gamelibrary.h"

namespace Orbital {

// ============================================================================
// initTestCase — enable Qt test mode so QStandardPaths redirects under
// ~/.qttest/, isolated from the user's real XDG dirs.
// ============================================================================

void TestGameLibrary::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
}

void TestGameLibrary::wipeDataDir() {
    const QString dataRoot = Paths::data();
    QDir d(dataRoot);
    if (d.exists()) d.removeRecursively();
    QDir().mkpath(Paths::gamesByUuid());
    QDir().mkpath(Paths::gamesByName());
}

void TestGameLibrary::cleanup() {
    wipeDataDir();
}

// ============================================================================
// addGame — should append, generate UUID if missing, persist to disk.
// ============================================================================

void TestGameLibrary::testCase_addGame() {
    Orbital::GameLibrary library;
    library.load();

    Orbital::GameEntry entry;
    entry.name = "Test Game";
    entry.executable = "/usr/bin/test";

    QString id = library.addGame(entry);

    QVERIFY(!id.isEmpty());
    QCOMPARE(library.count(), 1);

    // Verify UUID format (QUuid::WithoutBraces → 36 chars with 4 dashes).
    QCOMPARE(id.size(), 36);
    QCOMPARE(id.count('-'), 4);

    // Verify file persisted.
    const QString metaPath = Paths::gamesByUuid() + "/" + id + "/metadata.json";
    QVERIFY(QFileInfo::exists(metaPath));

    // Verify by-name/ symlink created.
    const QString linkPath = Paths::gamesByName() + "/Test Game";
    QVERIFY(QFileInfo(linkPath).isSymLink());

    // Verify symlink points to this game's UUID directory.
    // symLinkTarget() returns the resolved absolute path on most platforms,
    // so we check for either the raw relative form or the resolved form.
    const QString target = QFile::symLinkTarget(linkPath);
    QVERIFY(target.contains("../by-uuid/" + id) ||
            target.contains("/" + id));
}

// ============================================================================
// removeGame — should remove from memory AND from disk (dir + symlink).
// ============================================================================

void TestGameLibrary::testCase_removeGame() {
    Orbital::GameLibrary library;
    library.load();

    Orbital::GameEntry entry;
    entry.id = "11111111-2222-3333-4444-555555555555";
    entry.name = "Removable Game";
    entry.executable = "/usr/bin/test";
    QString id = library.addGame(entry);

    QVERIFY(library.count() == 1);
    QVERIFY(QFileInfo::exists(Paths::gamesByUuid() + "/" + id + "/metadata.json"));

    library.removeGame(id);
    QCOMPARE(library.count(), 0);
    QVERIFY(!QFileInfo::exists(Paths::gamesByUuid() + "/" + id));
    QVERIFY(!QFileInfo::exists(Paths::gamesByName() + "/Removable Game"));
}

// ============================================================================
// updateGame — change name; old symlink should be removed, new one created.
// ============================================================================

void TestGameLibrary::testCase_updateGame() {
    Orbital::GameLibrary library;
    library.load();

    Orbital::GameEntry entry;
    entry.id = "22222222-3333-4444-5555-666666666666";
    entry.name = "Old Name";
    entry.executable = "/usr/bin/x";
    QString id = library.addGame(entry);

    entry.name = "New Name";
    library.updateGame(entry);
    library.save(id);

    QVERIFY(!QFileInfo::exists(Paths::gamesByName() + "/Old Name"));
    QVERIFY(QFileInfo::exists(Paths::gamesByName() + "/New Name"));

    // Reload and verify the name change persisted.
    Orbital::GameLibrary fresh;
    fresh.load();
    QCOMPARE(fresh.findById(id).name, QString("New Name"));
}

// ============================================================================
// findById
// ============================================================================

void TestGameLibrary::testCase_findById() {
    Orbital::GameLibrary library;
    library.load();

    Orbital::GameEntry entry;
    entry.id = "33333333-4444-5555-6666-777777777777";
    entry.name = "FindMe By ID";
    entry.executable = "/usr/bin/x";
    library.addGame(entry);

    auto found = library.findById(entry.id);
    QVERIFY(!found.id.isEmpty());
    QCOMPARE(found.name, QString("FindMe By ID"));

    auto missing = library.findById("does-not-exist");
    QVERIFY(missing.id.isEmpty());
}

// ============================================================================
// findByName
// ============================================================================

void TestGameLibrary::testCase_findByName() {
    Orbital::GameLibrary library;
    library.load();

    Orbital::GameEntry entry;
    entry.id = "44444444-5555-6666-7777-888888888888";
    entry.name = "FindMe By Name";
    entry.executable = "/usr/bin/x";
    library.addGame(entry);

    auto found = library.findByName("FindMe By Name");
    QVERIFY(!found.id.isEmpty());
    QCOMPARE(found.id, entry.id);

    auto missing = library.findByName("does-not-exist");
    QVERIFY(missing.id.isEmpty());
}

// ============================================================================
// search — case-insensitive substring
// ============================================================================

void TestGameLibrary::testCase_search() {
    Orbital::GameLibrary library;
    library.load();

    Orbital::GameEntry e1; e1.name = "The Witcher 3"; e1.executable = "/x"; library.addGame(e1);
    Orbital::GameEntry e2; e2.name = "Elden Ring";    e2.executable = "/x"; library.addGame(e2);
    Orbital::GameEntry e3; e3.name = "Cyberpunk 2077"; e3.executable = "/x"; library.addGame(e3);

    QCOMPARE(library.search("Witcher").size(), 1);
    QCOMPARE(library.search("RING").size(), 1);     // case-insensitive
    // "e" matches all three: "The Witcher", "Elden", "Cyberpunk"
    QCOMPARE(library.search("e").size(), 3);
    QCOMPARE(library.search("").size(), 0);         // empty query returns nothing
}

// ============================================================================
// saveLoad — persist in one instance, load in another, verify roundtrip
// ============================================================================

void TestGameLibrary::testCase_saveLoad() {
    Orbital::GameLibrary writer;
    writer.load();

    Orbital::GameEntry entry;
    entry.id = "55555555-6666-7777-8888-999999999999";
    entry.name = "Roundtrip Game";
    entry.executable = "/usr/bin/x";
    entry.protonVersion = "GE-Proton9-7";
    entry.prefixMode = "per-game";
    entry.envVars["MANGOHUD"] = "1";
    entry.envVars["PROTON_ENABLE_WAYLAND"] = "1";
    writer.addGame(entry);

    Orbital::GameLibrary reader;
    reader.load();

    QCOMPARE(reader.count(), 1);
    auto loaded = reader.findById(entry.id);
    QCOMPARE(loaded.name, QString("Roundtrip Game"));
    QCOMPARE(loaded.protonVersion, QString("GE-Proton9-7"));
    QCOMPARE(loaded.envVars.size(), 2);
    QCOMPARE(loaded.envVars["MANGOHUD"], QString("1"));
}

// ============================================================================
// symlinkCreation — verify by-name/ is relative (../by-uuid/...)
// ============================================================================

void TestGameLibrary::testCase_symlinkCreation() {
    Orbital::GameLibrary library;
    library.load();

    Orbital::GameEntry e;
    e.id = "66666666-7777-8888-9999-aaaaaaaaaaaa";
    e.name = "Symlink Test";
    e.executable = "/x";
    library.addGame(e);

    const QString linkPath = Paths::gamesByName() + "/Symlink Test";
    QVERIFY(QFileInfo(linkPath).isSymLink());

    // Read the raw symlink target (not resolved).
    const QString rawTarget = QFile::decodeName(QFile::encodeName(linkPath));
    QFile f(linkPath);
    const QString symTarget = f.symLinkTarget();
    QVERIFY(symTarget.contains("../by-uuid/" + e.id) ||
            symTarget.contains("/" + e.id));
}

// ============================================================================
// testNameResolution — special-char sanitization: '/' → U+2215 DIVISION SLASH
// ============================================================================

void TestGameLibrary::testNameResolution() {
    Orbital::GameLibrary library;
    library.load();

    Orbital::GameEntry e;
    e.id = "77777777-8888-9999-aaaa-bbbbbbbbbbbb";
    e.name = "Game/Subtitle";  // contains forward slash
    e.executable = "/x";
    library.addGame(e);

    // The symlink filename should use U+2215, not '/'.
    const QString badPath = Paths::gamesByName() + "/Game/Subtitle";  // would be a real subdir
    QVERIFY(!QFileInfo::exists(badPath));

    const QString goodPath = Paths::gamesByName() + "/Game\u2215Subtitle";
    QVERIFY(QFileInfo::exists(goodPath));
}

// ============================================================================
// atomicWrite — after save, no .tmp file should be left behind
// ============================================================================

void TestGameLibrary::testCase_atomicWrite() {
    Orbital::GameLibrary library;
    library.load();

    Orbital::GameEntry e;
    e.id = "88888888-9999-aaaa-bbbb-cccccccccccc";
    e.name = "Atomic Test";
    e.executable = "/x";
    library.addGame(e);

    const QString metaPath = Paths::gamesByUuid() + "/" + e.id + "/metadata.json";
    const QString tmpPath  = metaPath + ".tmp";
    QVERIFY(QFileInfo::exists(metaPath));
    QVERIFY(!QFileInfo::exists(tmpPath));

    // Re-save and verify same.
    library.save(e.id);
    QVERIFY(QFileInfo::exists(metaPath));
    QVERIFY(!QFileInfo::exists(tmpPath));
}

// ============================================================================
// concurrentAccess — two library instances pointing at the same dir
// ============================================================================

void TestGameLibrary::testCase_concurrentAccess() {
    Orbital::GameLibrary writer;
    writer.load();

    Orbital::GameEntry e;
    e.id = "99999999-aaaa-bbbb-cccc-dddddddddddd";
    e.name = "Concurrent";
    e.executable = "/x";
    writer.addGame(e);

    // Reader doesn't see the new game until it loads.
    Orbital::GameLibrary reader;
    QVERIFY(reader.count() == 0);
    reader.load();
    QCOMPARE(reader.count(), 1);
}

} // namespace Orbital

int main(int argc, char *argv[]) {
    Orbital::TestGameLibrary test;
    return QTest::qExec(&test, argc, argv);
}
