#include "orbital/processlauncher.h"
#include <QProcess>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcProcessLauncher, "orbital.processlauncher")

namespace Orbital {

ProcessLauncher::ProcessLauncher(QObject *parent) : QObject(parent) {
    m_process = new QProcess(this);
}

ProcessLauncher::~ProcessLauncher() = default;

bool ProcessLauncher::launch(const LaunchConfig &config) {
    Q_UNUSED(config);
    qCInfo(lcProcessLauncher) << "Launch requested";
    return true;
}

bool ProcessLauncher::launchGame(const QString &gameId, const QString &protonPath,
                                  const QString &prefixPath) {
    Q_UNUSED(gameId);
    Q_UNUSED(protonPath);
    Q_UNUSED(prefixPath);
    qCInfo(lcProcessLauncher) << "Launch game requested:" << gameId;
    return true;
}

bool ProcessLauncher::validateEnvVars(const QMap<QString, QString> &envVars) const {
    Q_UNUSED(envVars);
    return true;
}

QProcessEnvironment ProcessLauncher::buildEnvironment(const LaunchConfig &config) const {
    Q_UNUSED(config);
    return QProcessEnvironment::systemEnvironment();
}

QStringList ProcessLauncher::filterDenyList() {
    return {
        "DISPLAY",
        "HOME",
        "USER",
        "PATH",
        "LANG",
        "LD_LIBRARY_PATH",
        "LD_PRELOAD"
    };
}

QProcessEnvironment ProcessLauncher::buildEnvOrderedList(const LaunchConfig &config) const {
    return buildEnvironment(config);
}

} // namespace Orbital
