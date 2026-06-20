#include <QDebug>

#include "parser.h"
#include "orbital/presetmanager.h"
#include "orbital/gamelibrary.h"
#include "orbital/paths.h"

int cmd_preset(int argc, const char *const *argv) {
    if (argc < 1) {
        printUsage("preset", "<subcommand> [args...]");
        qDebug() << "Available: list, apply, save, delete, refresh";
        return 1;
    }

    QString subcommand = QString::fromUtf8(argv[0]);
    Orbital::PresetManager manager;
    Orbital::GameLibrary library;

    manager.init();

    if (subcommand == "list") {
        bool ok = false;
        auto listArgs = PresetParser::parseList(argc, argv, ok);
        if (!ok) return 1;

        if (listArgs.gameName.isEmpty()) {
            printError("Game name cannot be empty");
            return 1;
        }

        auto presets = manager.presetsForGame(listArgs.gameName);
        qDebug() << "Presets for" << listArgs.gameName << "(" << presets.size() << "):";
        for (const auto &p : presets) {
            qDebug() << "  " << p.id << "-" << p.label;
            if (!p.description.isEmpty()) {
                qDebug() << "    " << p.description;
            }
            if (!p.envVars.isEmpty()) {
                qDebug() << "    Env vars:";
                for (auto it = p.envVars.constBegin(); it != p.envVars.constEnd(); ++it) {
                    qDebug() << "      " << it.key() << "=" << it.value();
                }
            }
        }
        return 0;

    } else if (subcommand == "apply") {
        bool ok = false;
        auto applyArgs = PresetParser::parseApply(argc, argv, ok);
        if (!ok) return 1;

        if (applyArgs.gameName.isEmpty() || applyArgs.presetId.isEmpty()) {
            printError("Game name and preset ID cannot be empty");
            return 1;
        }

        library.load();
        auto game = library.findByName(applyArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + applyArgs.gameName);
            return 1;
        }

        qDebug() << "Applying preset" << applyArgs.presetId << "to" << game.name;

        // TODO: Actually apply the preset to the game
        // auto merged = manager.mergePresets(...);
        // game.envVars = merged.envVars;
        // library.updateGame(game);
        // library.save(game.id);

        return 0;

    } else if (subcommand == "save") {
        bool ok = false;
        auto saveArgs = PresetParser::parseSave(argc, argv, ok);
        if (!ok) return 1;

        if (saveArgs.gameName.isEmpty()) {
            printError("Game name cannot be empty");
            return 1;
        }

        if (saveArgs.label.isEmpty()) {
            printError("Preset label cannot be empty");
            return 1;
        }

        library.load();
        auto game = library.findByName(saveArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + saveArgs.gameName);
            return 1;
        }

        qDebug() << "Saving preset" << saveArgs.label << "for" << game.name;

        // TODO: Actually save the preset
        // manager.saveLocalPreset(game, saveArgs.label);

        return 0;

    } else if (subcommand == "delete") {
        bool ok = false;
        auto deleteArgs = PresetParser::parseDelete(argc, argv, ok);
        if (!ok) return 1;

        if (deleteArgs.gameName.isEmpty() || deleteArgs.presetId.isEmpty()) {
            printError("Game name and preset ID cannot be empty");
            return 1;
        }

        library.load();
        auto game = library.findByName(deleteArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + deleteArgs.gameName);
            return 1;
        }

        qDebug() << "Deleting preset" << deleteArgs.presetId << "for" << game.name;
        manager.deleteLocalPreset(game.name, deleteArgs.presetId);
        qDebug() << "Preset deleted";
        return 0;

    } else if (subcommand == "refresh") {
        auto result = PresetParser::parse(argc, argv, nullptr);
        if (!result.valid) {
            printError(result.errorMessage);
            return 1;
        }

        qDebug() << "Refreshing preset cache...";
        manager.refresh();
        qDebug() << "Cache refreshed.";
        return 0;

    } else {
        printError("Unknown subcommand: " + subcommand);
        qDebug() << "Available: list, apply, save, delete, refresh";
        return 1;
    }
}
