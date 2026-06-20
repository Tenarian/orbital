#include <QUuid>
#include <QDebug>
#include <QFileInfo>
#include <QEventLoop>
#include <QTimer>

#include "parser.h"
#include "orbital/gamelibrary.h"
#include "orbital/paths.h"
#include "orbital/types.h"
#include "orbital/processlauncher.h"
#include "orbital/envvarschema.h"
#include "orbital/configstore.h"

// ============================================================================
// cmd_library — entry point for `orbital library <subcommand>`.
//
// Subcommands: list, search, add, launch, remove
// ============================================================================

int cmd_library(int argc, const char *const *argv) {
    if (argc < 1) {
        printUsage("library", "<subcommand> [args...]");
        qDebug() << "Available: list, search, add, launch, remove";
        return 1;
    }

    QString subcommand = QString::fromUtf8(argv[0]);

    // ---------------- list ----------------
    if (subcommand == "list") {
        // parseList has a different signature than the other parsers —
        // it takes (argc, argv) without an ok out-param.
        LibraryParser::parseList(argc, argv);

        Orbital::GameLibrary library;
        library.load();  // FIX: was missing — `list` always returned 0 games

        const auto games = library.allGames();
        qDebug() << "Games (" << games.size() << "):";
        for (const auto &game : games) {
            qDebug().noquote() << "  " << game.id << game.name
                                << "(" << game.executable << ")";
        }
        return 0;
    }

    // ---------------- search ----------------
    if (subcommand == "search") {
        bool ok = false;
        auto searchArgs = LibraryParser::parseSearch(argc, argv, ok);
        if (!ok) return 1;

        Orbital::GameLibrary library;
        library.load();  // FIX: was missing

        auto results = library.search(searchArgs.query);
        qDebug() << "Search results for '" << searchArgs.query << "' (" << results.size() << "):";
        for (const auto &game : results) {
            qDebug().noquote() << "  " << game.id << game.name;
        }
        return 0;
    }

    // ---------------- add ----------------
    if (subcommand == "add") {
        bool ok = false;
        auto addArgs = LibraryParser::parseAdd(argc, argv, ok);
        if (!ok) return 1;

        // Validate name is not empty
        if (addArgs.name.isEmpty()) {
            printError("Game name cannot be empty");
            return 1;
        }

        // Validate executable path is not empty
        if (addArgs.executable.isEmpty()) {
            printError("Executable path cannot be empty");
            return 1;
        }

        // Validate prefixMode
        if (addArgs.prefixMode != "per-game" && addArgs.prefixMode != "shared") {
            printWarning("Invalid prefix-mode '" + addArgs.prefixMode + "', defaulting to 'per-game'");
            addArgs.prefixMode = "per-game";
        }

        // Validate shared prefix ID when mode is shared
        if (addArgs.prefixMode == "shared" && addArgs.sharedPrefixId.isEmpty()) {
            printWarning("prefix-mode is 'shared' but no --shared-prefix specified");
        }

        Orbital::GameLibrary library;
        library.load();

        // Check for duplicate name
        auto existing = library.findByName(addArgs.name);
        if (!existing.id.isEmpty()) {
            printError("A game with name '" + addArgs.name + "' already exists (ID: " + existing.id + ")");
            return 1;
        }

        Orbital::GameEntry entry;
        // GameLibrary::addGame will auto-generate a UUID if entry.id is empty,
        // but we keep the explicit generation here so the message below can
        // print the new ID.
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.name = addArgs.name;
        entry.executable = addArgs.executable;
        entry.protonVersion = addArgs.protonVersion;
        entry.launchArgs = addArgs.launchArgs.join(" ");
        entry.prefixMode = addArgs.prefixMode;
        entry.sharedPrefixId = addArgs.sharedPrefixId;

        QString id = library.addGame(entry);  // addGame persists + creates by-name/ symlink
        qDebug().noquote() << "Added game:" << addArgs.name << "with ID:" << id;
        qDebug().noquote() << "  Metadata:" << Orbital::Paths::gamesByUuid() + "/" + id + "/metadata.json";
        return 0;
    }

    // ---------------- launch ----------------
    if (subcommand == "launch") {
        bool ok = false;
        auto launchArgs = LibraryParser::parseLaunch(argc, argv, ok);
        if (!ok) return 1;

        if (launchArgs.identifier.isEmpty()) {
            printError("Game identifier cannot be empty");
            return 1;
        }

        Orbital::GameLibrary library;
        library.load();

        // Look up by name first, then by UUID.
        auto game = library.findByName(launchArgs.identifier);
        if (game.id.isEmpty()) {
            game = library.findById(launchArgs.identifier);
        }
        if (game.id.isEmpty()) {
            printError("Game not found: " + launchArgs.identifier);
            return 1;
        }

        qDebug().noquote() << "Launching:" << game.name;
        qDebug().noquote() << "  Executable:" << game.executable;
        qDebug().noquote() << "  Proton:" << (game.protonVersion.isEmpty() ? QStringLiteral("(none)") : game.protonVersion);
        qDebug().noquote() << "  Prefix mode:" << game.prefixMode;

        // Resolve proton path. The user-supplied protonVersion is a label
        // (e.g. "GE-Proton9-7"); we look it up under
        // ~/.steam/root/compatibilitytools.d/ first, then under
        // steamapps/common/. If no version is set or it can't be found, we
        // bail out with a clear error.
        QString protonPath;
        if (!game.protonVersion.isEmpty()) {
            QStringList candidates = {
                Orbital::Paths::steamDir() + "/compatibilitytools.d/" + game.protonVersion + "/proton",
                Orbital::Paths::steamDir() + "/steamapps/common/" + game.protonVersion + "/proton",
            };
            for (const QString &p : candidates) {
                if (QFileInfo::exists(p)) {
                    protonPath = p;
                    break;
                }
            }
            if (protonPath.isEmpty()) {
                printError("Proton version '" + game.protonVersion + "' not found on disk.\n"
                           "  Looked in:\n    " + candidates.join("\n    "));
                return 1;
            }
        } else {
            printError("No Proton version set for this game. Use: orbital proton set \"" + game.name + "\" <version>");
            return 1;
        }

        // Resolve prefix path.
        QString prefixPath;
        if (game.prefixMode == "shared" && !game.sharedPrefixId.isEmpty()) {
            prefixPath = Orbital::Paths::prefixesShared() + "/" + game.sharedPrefixId;
        } else {
            prefixPath = Orbital::Paths::prefixesByGame() + "/" + game.id;
        }
        QDir().mkpath(prefixPath);

        // Build LaunchConfig.
        Orbital::LaunchConfig config;
        config.executable = game.executable;
        config.protonPath = protonPath;
        config.prefixPath = prefixPath;
        config.envVars = game.envVars;
        // Parse launch args (space-separated string → list).
        if (!game.launchArgs.isEmpty()) {
            config.args = game.launchArgs.split(' ', Qt::SkipEmptyParts);
        }

        // Wire schema loader for env-group expansion + validation.
        Orbital::EnvVarSchemaLoader schema;
        schema.load();
        Orbital::ProcessLauncher launcher;
        launcher.setSchemaLoader(&schema);

        // Validate env vars before launch.
        if (!launcher.validateEnvVars(config.envVars)) {
            printError("Cannot launch: one or more env vars are invalid. "
                       "Run `orbital env list \"" + game.name + "\"` to inspect.");
            return 1;
        }

        if (!launcher.launch(config)) {
            printError("Failed to start Proton process");
            return 1;
        }

        // Block until the process exits (CLI mode — no event loop spin-up
        // beyond this wait). For long-running games this just waits; for
        // failures the error signal fires immediately.
        QEventLoop loop;
        QObject::connect(&launcher, &Orbital::ProcessLauncher::processFinished,
                         &loop, &QEventLoop::quit);
        QObject::connect(&launcher, &Orbital::ProcessLauncher::processError,
                         &loop, &QEventLoop::quit);
        loop.exec();

        // Update lastPlayed timestamp on successful exit.
        game.lastPlayed = QDateTime::currentSecsSinceEpoch();
        library.updateGame(game);
        library.save(game.id);

        qDebug().noquote() << "Game exited. lastPlayed updated.";
        return 0;
    }

    // ---------------- remove ----------------
    if (subcommand == "remove") {
        bool ok = false;
        auto removeArgs = LibraryParser::parseRemove(argc, argv, ok);
        if (!ok) return 1;

        // Validate ID format (should be a UUID)
        QUuid uuid(removeArgs.id);
        if (uuid.isNull()) {
            printError("Invalid game ID format (expected UUID):" + removeArgs.id);
            return 1;
        }

        Orbital::GameLibrary library;
        library.load();

        auto game = library.findById(removeArgs.id);
        if (game.id.isEmpty()) {
            printError("Game not found: " + removeArgs.id);
            return 1;
        }

        library.removeGame(removeArgs.id);  // removeGame also deletes from disk + symlink
        qDebug().noquote() << "Removed game:" << game.name << "(" << removeArgs.id << ")";
        return 0;
    }

    // Unknown
    printError("Unknown subcommand: " + subcommand);
    qDebug() << "Available: list, search, add, launch, remove";
    return 1;
}
