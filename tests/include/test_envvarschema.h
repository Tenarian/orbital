#pragma once

#include <QtTest>
#include <QString>
#include <QTemporaryDir>
#include <QJsonObject>

namespace Orbital {

class TestEnvVarSchema : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void testCase_loadBuiltIn();
    void testCase_loadUserSchema();
    void testCase_entriesForCategory();
    void testCase_entryForKey();
    void testCase_validateValue();
    void testCase_parseValue();
    void testCase_envGroup();
    void testCase_reload();
    void testCase_invalidSchema();

private:
    QTemporaryDir m_tempDir;
    void createTestSchema(const QString &filename, const QJsonObject &schema);
};

} // namespace Orbital
