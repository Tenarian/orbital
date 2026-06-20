#include <QUuid>
#include <QDebug>

#include "parser.h"
#include "orbital/gamelibrary.h"
#include "orbital/paths.h"
#include "orbital/types.h"
#include "orbital/processlauncher.h"

int cmd_library(int argc, const char *const *argv) {
    // Parse and validate the subcommand
    ParserResult result;
    void *typedData = nullptr;

    if (argc < 1) {
        printUsage("library", "<subcommand> [args...]");
        qDebug() << "Available: list, search, add, launch, remove";
        return 1;
    }

    QString subcommand = QString::fromUtf8(argv[0]);

    if (subcommand == "list") {
        result = LibraryParser::parse(argc, argv, &typedData);
        if (!result.valid) {
            printError(result.errorMessage);
            return 1;
        }

    } else if (subcommand == "search") {
        result = LibraryParser::parse(argc, argv, &typedData);
        if (!result.valid) {
            printError(result.errorMessage);
            return 1;
        }

    } else if (subcommand == "add") {
        result = LibraryParser::parse(argc, argv, &typedData);
        if (!result.valid) {
            printError(result.errorMessage);
            return 1;
        }

    } else if (subcommand == "launch") {
        result = LibraryParser::parse(argc, argv, &typedData);
        if (!result.valid) {
            printError(result.errorMessage);
            return 1;
        }

    } else if (subcommand == "remove") {
        result = LibraryParser::parse(argc, argv, &typedData);
        if (!result.valid) {
            printError(result.errorMessage);
            return 1;
        }

    } else {
        printError("Unknown subcommand: " + subcommand);
        qDebug() << "Available: list, search, add, launch, remove";
        return 1;
    }

    // Dispatch on subcommand with typed data
    if (subcommand == "list") {
        auto games = Orbital::GameLibrary().allGames();
        qDebug() << "Games (" << games.size() << "):";
        for (const auto &game : games) {
            qDebug() << "  " << game.id << "-" << game.name << "(" << game.executable << ")";
        }
        return 0;

    } else if (subcommand == "search") {
        bool ok = false;
        auto searchArgs = LibraryParser::parseSearch(argc, argv, ok);
        if (!ok) return 1;

        auto library = Orbital::GameLibrary();
        auto results = library.search(searchArgs.query);
        qDebug() << "Search results for '" << searchArgs.query << "' (" << results.size() << "):";
        for (const auto &game : results) {
            qDebug() << "  " << game.id << "-" << game.name;
        }
        return 0;

    } else if (subcommand == "add") {
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

        auto library = Orbital::GameLibrary();
        library.load();

        // Check for duplicate name
        auto existing = library.findByName(addArgs.name);
        if (!existing.id.isEmpty()) {
            printError("A game with name '" + addArgs.name + "' already exists (ID: " + existing.id + ")");
            return 1;
        }

        Orbital::GameEntry entry;
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.name = addArgs.name;
        entry.executable = addArgs.executable;
        entry.protonVersion = addArgs.protonVersion;
        entry.launchArgs = addArgs.launchArgs.join(" ");
        entry.prefixMode = addArgs.prefixMode;
        entry.sharedPrefixId = addArgs.sharedPrefixId;

        QString id = library.addGame(entry);
        library.save(id);
        qDebug() << "Added game:" << addArgs.name << "with ID:" << id;
        return 0;

    } else if (subcommand == "launch") {
        bool ok = false;
        auto launchArgs = LibraryParser::parseLaunch(argc, argv, ok);
        if (!ok) return 1;

        // Validate identifier
        if (launchArgs.identifier.isEmpty()) {
            printError("Game identifier cannot be empty");
            return 1;
        }

        auto library = Orbital::GameLibrary();
        library.load();

        // Look up by name first, then by UUID
        auto game = library.findByName(launchArgs.identifier);
        if (game.id.isEmpty()) {
            game = library.findById(launchArgs.identifier);
        }

        if (game.id.isEmpty()) {
            printError("Game not found: " + launchArgs.identifier);
            return 1;
        }

        qDebug() << "Launching:" << game.name;
        qDebug() << "Executable:" << game.executable;
        qDebug() << "Proton:" << game.protonVersion;
        qDebug() << "Prefix:" << game.prefixMode;

        // TODO: Actually launch via ProcessLauncher
        // Orbital::ProcessLauncher launcher;
        // Orbital::LaunchConfig config;
        // config.executable = game.executable;
        // config.protonPath = ...;
        // config.prefixPath = ...;
        // config.envVars = game.envVars;
        // launcher.launch(config);

        return 0;

    } else if (subcommand == "remove") {
        bool ok = false;
        auto removeArgs = LibraryParser::parseRemove(argc, argv, ok);
        if (!ok) return 1;

        // Validate ID format (should be a UUID)
        QUuid uuid(removeArgs.id);
        if (uuid.isNull()) {
            printError("Invalid game ID format (expected UUID):" + removeArgs.id);
            return 1;
        }

        auto library = Orbital::GameLibrary();
        library.load();

        auto game = library.findById(removeArgs.id);
        if (game.id.isEmpty()) {
            printError("Game not found: " + removeArgs.id);
            return 1;
        }

        library.removeGame(removeArgs.id);
        library.save(removeArgs.id);
        qDebug() << "Removed game:" << game.name << "(" << removeArgs.id << ")";
        return 0;

    }

    return 1;
}
