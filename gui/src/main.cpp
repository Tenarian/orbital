#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcGUI, "orbital.gui")

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("orbital-gui");
    QCoreApplication::setApplicationVersion("0.1.0");

    QQmlApplicationEngine engine;
    // Register QML types
    engine.setImportPathList({":/gui", "qrc:/gui"});

    // Try loading from executable directory, then source tree
    QString exeDir = QCoreApplication::applicationDirPath();
    QString srcDir = QDir(exeDir).absoluteFilePath("../..");
    QStringList candidates = {
        exeDir,
        srcDir,
        QDir(srcDir).absoluteFilePath("gui"),
    };
    bool loaded = false;
    for (const auto &dir : candidates) {
        QString qmlFile = QDir(dir).absoluteFilePath("qml/main.qml");
        if (QFile::exists(qmlFile)) {
            engine.load(QUrl::fromLocalFile(qmlFile));
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        qCWarning(lcGUI) << "Could not load QML from:" << candidates;
        return -1;
    }

    return app.exec();
}
