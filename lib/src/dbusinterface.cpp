#include "orbital/dbusinterface.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcDBus, "orbital.dbus")

namespace Orbital {

DBusInterface::DBusInterface(QObject *parent)
    : QObject(parent), m_dbus(QDBusConnection::sessionBus()) {}

DBusInterface::~DBusInterface() = default;

bool DBusInterface::registerService() {
    m_dbus = QDBusConnection::sessionBus();
    bool registered = m_dbus.registerService("io.orbital.Launcher");
    qCInfo(lcDBus) << "DBus service registered:" << registered;
    return registered;
}

void DBusInterface::unregisterService() {
    m_dbus.unregisterService("io.orbital.Launcher");
    qCInfo(lcDBus) << "DBus service unregistered";
}

QVariantList DBusInterface::listGames() {
    QVariantList result;
    qCInfo(lcDBus) << "listGames called";
    return result;
}

bool DBusInterface::launchGame(const QString &gameId) {
    qCInfo(lcDBus) << "launchGame called:" << gameId;
    return m_launcher.launchGame(gameId);
}

void DBusInterface::onGameAdded(const QString &id) {
    Q_UNUSED(id);
}

void DBusInterface::onGameRemoved(const QString &id) {
    Q_UNUSED(id);
}

void DBusInterface::onGameUpdated(const QString &id) {
    Q_UNUSED(id);
}

void DBusInterface::onProcessStarted() {
}

void DBusInterface::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
}

void DBusInterface::onProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error);
}

bool DBusInterface::registerObject() {
    return true;
}

bool DBusInterface::unregisterObject() {
    return true;
}

} // namespace Orbital
