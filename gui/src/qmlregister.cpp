#include <QQmlEngine>
#include <QQmlApplicationEngine>
#include <QtQml>

#include "bridge/protonmodel.h"
#include "bridge/librarymodel.h"
#include "bridge/envvarmodel.h"

// Forward declarations for QML type registration
QML_DECLARE_TYPE(Orbital::ProtonModel)
QML_DECLARE_TYPE(Orbital::LibraryModel)
QML_DECLARE_TYPE(Orbital::EnvVarModel)

// QML import directory setup
static void register_qml_types(const QString &importPath) {
    const char *uri = importPath.toUtf8().constData();
    qmlRegisterType<Orbital::ProtonModel>(uri, 1, 0, "ProtonModel");
    qmlRegisterType<Orbital::LibraryModel>(uri, 1, 0, "LibraryModel");
    qmlRegisterType<Orbital::EnvVarModel>(uri, 1, 0, "EnvVarModel");
}

// Called from main.cpp after engine creation
inline void setupQmlEngine(QQmlApplicationEngine &engine) {
    register_qml_types("Orbital");
    engine.setImportPathList({":/gui", "qrc:/gui"});
}
