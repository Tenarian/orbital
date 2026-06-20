#pragma once

#include <QObject>
#include <QString>
#include <QDBusConnection>
#include <QVariantList>

#include "orbital/gamelibrary.h"
#include "orbital/processlauncher.h"
#include "orbital/types.h"

namespace Orbital {

class DBusInterface : public QObject {
    Q_OBJECT

public:
    explicit DBusInterface(QObject *parent = nullptr);
    ~DBusInterface();

    // Register the D-Bus service
    bool registerService();

    // Unregister the D-Bus service
    void unregisterService();

public slots:
    // List all games (returns array of game objects)
    QVariantList listGames();

    // Launch a game by ID
    bool launchGame(const QString &gameId);

private slots:
    void onGameAdded(const QString &id);
    void onGameRemoved(const QString &id);
    void onGameUpdated(const QString &id);
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    GameLibrary m_library;
    ProcessLauncher m_launcher;
    QDBusConnection m_dbus;
    int m_connectionCookie = 0;

    // D-Bus registration helpers
    bool registerObject();
    bool unregisterObject();
};

} // namespace Orbital
