#pragma once

#include <QtTest>
#include <QString>
#include <QList>
#include <orbital/presetmanager.h>

namespace Orbital {

class TestPresetMerge : public QObject {
    Q_OBJECT

private slots:
    void testCase_mergeTwoPresets();
    void testCase_mergeThreePresets();
    void testCase_conflictResolution();
    void testCase_noConflict();
    void testCase_emptyPresets();
    void testCase_singlePreset();

private:
    QList<Preset> createTestPresets();
};

} // namespace Orbital
