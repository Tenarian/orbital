#include "orbital/paths.h"

#include <QDir>
#include <QStandardPaths>

namespace Orbital {

// Per PRD §XDG Directory Layout, all paths should be:
//   ~/.config/orbital/         (XDG_CONFIG_HOME/orbital/)
//   ~/.local/share/orbital/    (XDG_DATA_HOME/orbital/)
//   ~/.cache/orbital/          (XDG_CACHE_HOME/orbital/)
//
// We use the Generic* locations and append "/orbital" explicitly so the
// path is independent of QCoreApplication::organizationName() / applicationName()
// (which would produce doubled "orbital/orbital/" prefixes otherwise).

QString Paths::resolvedConfigDir() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/orbital";
}

QString Paths::resolvedDataDir() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/orbital";
}

QString Paths::resolvedCacheDir() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/orbital";
}

QString Paths::config() {
    return resolvedConfigDir();
}

QString Paths::data() {
    return resolvedDataDir();
}

QString Paths::cache() {
    return resolvedCacheDir();
}

QString Paths::games() {
    return data() + "/games/by-uuid";
}

QString Paths::gamesByName() {
    return data() + "/games/by-name";
}

QString Paths::prefixes() {
    return data() + "/prefixes";
}

QString Paths::artwork() {
    return cache() + "/artwork";
}

QString Paths::hltbCache() {
    return cache() + "/hltb";
}

QString Paths::protonDb() {
    return cache() + "/protondb";
}

QString Paths::presets() {
    return cache() + "/presets";
}

QString Paths::userSchemas() {
    return config() + "/schema";
}

QString Paths::builtinSchemas() {
    QString dataDir = QStandardPaths::locate(
        QStandardPaths::GenericDataLocation,
        "orbital/schema",
        QStandardPaths::LocateDirectory
    );
    if (dataDir.isEmpty()) {
        dataDir = qgetenv("ORBITAL_DATA_DIR");
        if (dataDir.isEmpty()) {
            dataDir = ".";
        }
        dataDir += "/assets/schema";
    }
    return dataDir;
}

QString Paths::gamesByUuid() {
    return data() + "/games/by-uuid";
}

QString Paths::prefixesByGame() {
    return data() + "/prefixes/by-game";
}

QString Paths::prefixesShared() {
    return data() + "/prefixes/shared";
}

QString Paths::presetsGames() {
    return cache() + "/presets/games";
}

QString Paths::userSchemaCustom() {
    return config() + "/schema/custom.json";
}

QString Paths::configFile() {
    return config() + "/config.json";
}

QString Paths::steamDir() {
    // Try XDG data location for Steam
    QString steamPath = QStandardPaths::locate(
        QStandardPaths::GenericDataLocation,
        "Steam",
        QStandardPaths::LocateDirectory
    );
    if (!steamPath.isEmpty()) {
        return steamPath;
    }
    // Default Steam installation directory on Linux
    QString home = QDir::homePath();
    QString defaultPath = home + "/.local/share/Steam";
    if (QDir(defaultPath).exists()) {
        return defaultPath;
    }
    return defaultPath; // Return even if it doesn't exist yet
}

} // namespace Orbital
