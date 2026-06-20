#include <QDebug>

#include "parser.h"
#include "orbital/prefixmanager.h"
#include "orbital/gamelibrary.h"
#include "orbital/paths.h"

int cmd_prefix(int argc, const char *const *argv) {
    if (argc < 1) {
        printUsage("prefix", "<subcommand> [args...]");
        qDebug() << "Available: open, reset, run, shared";
        return 1;
    }

    QString subcommand = QString::fromUtf8(argv[0]);
    Orbital::PrefixManager manager;
    Orbital::GameLibrary library;

    if (subcommand == "open") {
        bool ok = false;
        auto openArgs = PrefixParser::parseOpen(argc, argv, ok);
        if (!ok) return 1;

        if (openArgs.gameName.isEmpty()) {
            printError("Game name cannot be empty");
            return 1;
        }

        library.load();
        auto game = library.findByName(openArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + openArgs.gameName);
            return 1;
        }

        QString prefixPath = manager.prefixPath(game.id);
        qDebug() << "Opening prefix for" << game.name << ":" << prefixPath;

        // TODO: Actually open file manager
        // QDesktopServices::openUrl(QUrl::fromLocalFile(prefixPath));

        return 0;

    } else if (subcommand == "reset") {
        bool ok = false;
        auto resetArgs = PrefixParser::parseReset(argc, argv, ok);
        if (!ok) return 1;

        if (resetArgs.gameName.isEmpty()) {
            printError("Game name cannot be empty");
            return 1;
        }

        library.load();
        auto game = library.findByName(resetArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + resetArgs.gameName);
            return 1;
        }

        qDebug() << "Resetting prefix for" << game.name;
        manager.resetPrefix(game.id);
        qDebug() << "Prefix reset for" << game.name;
        return 0;

    } else if (subcommand == "run") {
        bool ok = false;
        auto runArgs = PrefixParser::parseRun(argc, argv, ok);
        if (!ok) return 1;

        if (runArgs.gameName.isEmpty() || runArgs.command.isEmpty()) {
            printError("Game name and command cannot be empty");
            return 1;
        }

        library.load();
        auto game = library.findByName(runArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + runArgs.gameName);
            return 1;
        }

        QString prefixPath = manager.prefixPath(game.id);
        qDebug() << "Running" << runArgs.command;
        if (!runArgs.args.isEmpty()) {
            qDebug() << "Args:" << runArgs.args;
        }
        qDebug() << "in prefix for" << game.name;
        qDebug() << "Prefix path:" << prefixPath;

        // TODO: Actually execute command in prefix
        // Use Proton/WINE to run the command

        return 0;

    } else if (subcommand == "shared") {
        // Delegate to shared sub-parsers
        if (argc < 2) {
            printError("shared requires a subcommand: list, create, assign, run");
            printUsage("prefix shared", "<list|create|assign|run> [args...]");
            return 1;
        }

        QString sharedSub = QString::fromUtf8(argv[1]);

        if (sharedSub == "list") {
            bool ok = false;
            auto sharedArgs = PrefixParser::parseSharedList(argc - 1, argv + 1, ok);
            if (!ok) return 1;

            auto prefixes = manager.listSharedPrefixes();
            qDebug() << "Shared prefixes (" << prefixes.size() << "):";
            for (const auto &p : prefixes) {
                qDebug() << "  " << p.id << "-" << p.name << "(" << p.games.size() << " games)";
            }
            return 0;

        } else if (sharedSub == "create") {
            bool ok = false;
            auto createArgs = PrefixParser::parseSharedCreate(argc - 1, argv + 1, ok);
            if (!ok) return 1;

            if (createArgs.id.isEmpty() || createArgs.name.isEmpty()) {
                printError("Shared prefix ID and name cannot be empty");
                return 1;
            }

            qDebug() << "Creating shared prefix:" << createArgs.id << "-" << createArgs.name;
            if (!createArgs.protonVersion.isEmpty()) {
                qDebug() << "Proton version:" << createArgs.protonVersion;
            }

            // TODO: Actually create the shared prefix
            // PrefixMeta meta;
            // meta.id = createArgs.id;
            // meta.name = createArgs.name;
            // meta.protonVersion = createArgs.protonVersion;
            // manager.createSharedPrefix(meta);

            return 0;

        } else if (sharedSub == "assign") {
            bool ok = false;
            auto assignArgs = PrefixParser::parseSharedAssign(argc - 1, argv + 1, ok);
            if (!ok) return 1;

            if (assignArgs.gameName.isEmpty() || assignArgs.prefixId.isEmpty()) {
                printError("Game name and prefix ID cannot be empty");
                return 1;
            }

            library.load();
            auto game = library.findByName(assignArgs.gameName);
            if (game.id.isEmpty()) {
                printError("Game not found: " + assignArgs.gameName);
                return 1;
            }

            qDebug() << "Assigning" << game.name << "to shared prefix" << assignArgs.prefixId;

            // TODO: Actually assign the game to the shared prefix

            return 0;

        } else if (sharedSub == "run") {
            bool ok = false;
            auto runArgs = PrefixParser::parseSharedRun(argc - 1, argv + 1, ok);
            if (!ok) return 1;

            if (runArgs.prefixId.isEmpty() || runArgs.command.isEmpty()) {
                printError("Prefix ID and command cannot be empty");
                return 1;
            }

            qDebug() << "Running" << runArgs.command;
            if (!runArgs.args.isEmpty()) {
                qDebug() << "Args:" << runArgs.args;
            }
            qDebug() << "in shared prefix" << runArgs.prefixId;

            // TODO: Actually execute command in shared prefix

            return 0;

        } else {
            printError("Unknown shared subcommand: " + sharedSub);
            qDebug() << "Available: list, create, assign, run";
            return 1;
        }

    } else {
        printError("Unknown subcommand: " + subcommand);
        qDebug() << "Available: open, reset, run, shared";
        return 1;
    }
}
