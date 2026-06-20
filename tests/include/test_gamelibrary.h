#pragma once

#include <QtTest>
#include <QString>
#include <QTemporaryDir>

namespace Orbital {

class TestGameLibrary : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();          // wipe test data dir between tests
    void testCase_addGame();
    void testCase_removeGame();
    void testCase_updateGame();
    void testCase_findById();
    void testCase_findByName();
    void testCase_search();
    void testCase_saveLoad();
    void testCase_symlinkCreation();
    void testNameResolution();
    void testCase_atomicWrite();
    void testCase_concurrentAccess();

private:
    QTemporaryDir m_tempDir;
    void wipeDataDir();
};

} // namespace Orbital
