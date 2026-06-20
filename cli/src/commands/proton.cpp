#include <QDebug>

#include "parser.h"
#include "orbital/protonmanager.h"
#include "orbital/gamelibrary.h"
#include "orbital/paths.h"

int cmd_proton(int argc, const char *const *argv) {
    if (argc < 1) {
        printUsage("proton", "<subcommand> [args...]");
        qDebug() << "Available: list, suggest, set, download, delete";
        return 1;
    }

    QString subcommand = QString::fromUtf8(argv[0]);
    Orbital::ProtonManager manager;

    if (subcommand == "list") {
        auto result = ProtonParser::parse(argc, argv, nullptr);
        if (!result.valid) {
            printError(result.errorMessage);
            return 1;
        }

        auto versions = manager.installedVersions();
        qDebug() << "Installed Proton versions (" << versions.size() << "):";
        for (const auto &v : versions) {
            qDebug() << "  " << v;
        }
        return 0;

    } else if (subcommand == "suggest") {
        bool ok = false;
        auto suggestArgs = ProtonParser::parseSuggest(argc, argv, ok);
        if (!ok) return 1;

        if (suggestArgs.gameName.isEmpty()) {
            printError("Game name cannot be empty");
            return 1;
        }

        QString suggestion = manager.suggestVersion(suggestArgs.gameName);
        if (suggestion.isEmpty()) {
            printWarning("No suggestion available for: " + suggestArgs.gameName);
        } else {
            qDebug() << "Suggested Proton for" << suggestArgs.gameName << ":" << suggestion;
        }
        return 0;

    } else if (subcommand == "set") {
        bool ok = false;
        auto setArgs = ProtonParser::parseSet(argc, argv, ok);
        if (!ok) return 1;

        if (setArgs.gameName.isEmpty() || setArgs.version.isEmpty()) {
            printError("Game name and proton version cannot be empty");
            return 1;
        }

        auto library = Orbital::GameLibrary();
        library.load();

        auto game = library.findByName(setArgs.gameName);
        if (game.id.isEmpty()) {
            printError("Game not found: " + setArgs.gameName);
            return 1;
        }

        // Validate version format
        if (setArgs.version.isEmpty()) {
            printError("Proton version cannot be empty");
            return 1;
        }

        qDebug() << "Setting Proton" << setArgs.version << "for" << game.name;

        // Update game entry with new proton version
        game.protonVersion = setArgs.version;
        library.updateGame(game);
        library.save(game.id);
        qDebug() << "Proton version updated for" << game.name;
        return 0;

    } else if (subcommand == "download") {
        bool ok = false;
        auto downloadArgs = ProtonParser::parseDownload(argc, argv, ok);
        if (!ok) return 1;

        if (downloadArgs.version.isEmpty()) {
            printError("Proton version cannot be empty");
            return 1;
        }

        qDebug() << "Downloading Proton" << downloadArgs.version;
        manager.downloadVersion(downloadArgs.version);
        return 0;

    } else if (subcommand == "delete") {
        bool ok = false;
        auto deleteArgs = ProtonParser::parseDelete(argc, argv, ok);
        if (!ok) return 1;

        if (deleteArgs.version.isEmpty()) {
            printError("Proton version cannot be empty");
            return 1;
        }

        qDebug() << "Deleting Proton" << deleteArgs.version;
        manager.deleteVersion(deleteArgs.version);
        return 0;

    } else {
        printError("Unknown subcommand: " + subcommand);
        qDebug() << "Available: list, suggest, set, download, delete";
        return 1;
    }
}
