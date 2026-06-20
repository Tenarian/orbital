#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcessEnvironment>
#include <QProcess>

#include "orbital/types.h"
#include "orbital/paths.h"

namespace Orbital {

class ProcessLauncher : public QObject {
    Q_OBJECT

public:
    explicit ProcessLauncher(QObject *parent = nullptr);
    ~ProcessLauncher() override;

    // Launch a game with the given config
    bool launch(const LaunchConfig &config);

    // Launch with a game ID (resolves from GameLibrary)
    bool launchGame(const QString &gameId, const QString &protonPath = {},
                    const QString &prefixPath = {});

    // Check if env vars are valid (no Invalid state)
    bool validateEnvVars(const QMap<QString, QString> &envVars) const;

    // Build environment for launch
    QProcessEnvironment buildEnvironment(const LaunchConfig &config) const;

    // Get the QProcess pointer (for monitoring)
    QProcess *process() const { return m_process; }

signals:
    void processStarted();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);
    void processOutput(const QString &output);

private:
    QProcess *m_process = nullptr;

    // Static deny-list for filtering system env
    static QStringList filterDenyList();

    // Build env in the specified order (later entries win)
    QProcessEnvironment buildEnvOrderedList(const LaunchConfig &config) const;
};

} // namespace Orbital
