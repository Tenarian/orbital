#include <QDebug>

#include "parser.h"
#include "orbital/envvarschema.h"
#include "orbital/gamelibrary.h"
#include "orbital/paths.h"

int cmd_env(int argc, const char *const *argv) {
    if (argc < 1) {
        printUsage("env", "<subcommand> [args...]");
        qDebug() << "Available: set, list, get, validate";
        return 1;
    }

    QString subcommand = QString::fromUtf8(argv[0]);
    Orbital::EnvVarSchemaLoader loader;
    Orbital::GameLibrary library;

    loader.load();

    if (subcommand == "set") {
        bool ok = false;
        auto setArgs = EnvParser::parseSet(argc, argv, ok);
        if (!ok) return 1;

        if (setArgs.gameName.isEmpty() || setArgs.key.isEmpty() || setArgs.value.isEmpty()) {
            printError("Game name, key, and value cannot be empty");
            return 1;
        }

        library.load();
        auto game = library.findByName(setArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + setArgs.gameName);
            return 1;
        }

        // Validate value against schema
        auto result = loader.validate(setArgs.key, setArgs.value);
        if (!result.valid) {
            printError("Invalid value for " + setArgs.key + ": " + result.errorMessage);
            return 1;
        }

        qDebug() << "Setting" << setArgs.key << "=" << setArgs.value << "for" << game.name;

        // TODO: Actually persist the env var to the game entry
        // game.envVars[setArgs.key] = setArgs.value;
        // library.updateGame(game);
        // library.save(game.id);

        return 0;

    } else if (subcommand == "list") {
        bool ok = false;
        auto listArgs = EnvParser::parseList(argc, argv, ok);
        if (!ok) return 1;

        if (listArgs.gameName.isEmpty()) {
            printError("Game name cannot be empty");
            return 1;
        }

        library.load();
        auto game = library.findByName(listArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + listArgs.gameName);
            return 1;
        }

        qDebug() << "Environment variables for" << game.name << ":";
        if (game.envVars.isEmpty()) {
            qDebug() << "  (none)";
        } else {
            for (auto it = game.envVars.constBegin(); it != game.envVars.constEnd(); ++it) {
                qDebug() << "  " << it.key() << "=" << it.value();
            }
        }
        return 0;

    } else if (subcommand == "get") {
        bool ok = false;
        auto getArgs = EnvParser::parseGet(argc, argv, ok);
        if (!ok) return 1;

        if (getArgs.gameName.isEmpty() || getArgs.key.isEmpty()) {
            printError("Game name and key cannot be empty");
            return 1;
        }

        library.load();
        auto game = library.findByName(getArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + getArgs.gameName);
            return 1;
        }

        auto it = game.envVars.find(getArgs.key);
        if (it == game.envVars.end()) {
            printError("Key not found: " + getArgs.key);
            return 1;
        }

        qDebug() << getArgs.key << "=" << it.value();
        return 0;

    } else if (subcommand == "validate") {
        bool ok = false;
        auto validateArgs = EnvParser::parseValidate(argc, argv, ok);
        if (!ok) return 1;

        if (validateArgs.key.isEmpty() || validateArgs.value.isEmpty()) {
            printError("Key and value cannot be empty");
            return 1;
        }

        auto result = loader.validate(validateArgs.key, validateArgs.value);
        if (result.valid) {
            qDebug() << "Valid:" << validateArgs.key << "=" << validateArgs.value;
            return 0;
        } else {
            printError("Invalid: " + validateArgs.key + "=" + validateArgs.value);
            printError("Error: " + result.errorMessage);
            return 1;
        }

    } else {
        printError("Unknown subcommand: " + subcommand);
        qDebug() << "Available: set, list, get, validate";
        return 1;
    }
}
