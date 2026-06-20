# Qt6 / Meson Build Patterns

## Q_DECLARE_LOGGING_CATEGORY vs Q_LOGGING_CATEGORY
- Header: Q_DECLARE_LOGGING_CATEGORY(name) — external reference only
- One .cpp: Q_LOGGING_CATEGORY(name, "tag") + #include <QLoggingCategory>
- Why: Q_DECLARE_LOGGING_CATEGORY alone leaves undefined reference

## QDBusConnection default constructor removed in Qt6
- Always initialize with QDBusConnection::sessionBus() in member initializer list
- Cannot write QDBusConnection() and assign later

## qmlRegisterType requires const char* URI, not QString
- Convert: const char *uri = importPath.toUtf8().constData();
- qmlRegisterType<MyType>(uri, 1, 0, "MyType");

## QTest + Q_OBJECT requires MOC
- Remove Q_OBJECT and write manual main() calling QTest::qExec()
- Or add .moc file explicitly via target_sources

## QVERIFY cannot cast void to bool
- Split void calls from assertions
- library.save("test-id"); QVERIFY(true);

## Namespace qualification in headers
- Use full namespace: QList<Orbital::Preset> not QList<Preset>

## Meson find_package(Qt6 ... COMPONENTS ...) preferred
- dependency('qt6') is deprecated
- Use find_package with explicit components

## All declared methods must have implementations
- undefined reference if method declared but not defined

## Check Qt6 component availability before use
- Not all Qt6 packages include all components (DBus, Quick, etc.)
