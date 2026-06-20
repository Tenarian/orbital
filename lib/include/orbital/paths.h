#pragma once

#include <QString>
#include <QStandardPaths>

namespace Orbital {

class Paths {
public:
    // Base XDG paths
    static QString config();
    static QString data();
    static QString cache();

    // Derived paths — all computed, never hardcoded elsewhere
    static QString games();
    static QString gamesByName();
    static QString prefixes();
    static QString artwork();
    static QString hltbCache();
    static QString protonDb();
    static QString presets();
    static QString userSchemas();
    static QString builtinSchemas();

    // Steam/Proton paths
    static QString steamDir();

    // Additional derived paths
    static QString gamesByUuid();
    static QString prefixesByGame();
    static QString prefixesShared();
    static QString presetsGames();
    static QString userSchemaCustom();
    static QString configFile();

private:
    static QString resolvedDataDir();
    static QString resolvedConfigDir();
    static QString resolvedCacheDir();
};

} // namespace Orbital
