#include "parser.h"

#include <QCoreApplication>
#include <QDebug>
#include <QStringList>

#include "orbital/gamelibrary.h"
#include "orbital/paths.h"
#include "orbital/processlauncher.h"
#include "orbital/protonmanager.h"
#include "orbital/prefixmanager.h"
#include "orbital/envvarschema.h"
#include "orbital/presetmanager.h"

// ============================================================================
// Helper utilities (defined here, declared in parser.h)
// ============================================================================

void printUsage(const QString &command, const QString &help) {
    qDebug() << "Usage: orbital" << command << help;
}

void printError(const QString &msg) {
    qDebug() << "Error:" << msg;
}

void printWarning(const QString &msg) {
    qDebug() << "Warning:" << msg;
}

// ============================================================================
// MainParser
// ============================================================================

ParserResult MainParser::parse(int argc, const char *const *argv) {
    ParserResult result;

    if (argc < 1) {
        result.errorMessage = "Missing command. Available: " + knownCommands().join(", ");
        return result;
    }

    result.command = QString::fromUtf8(argv[0]);
    result.remainingArgs = QStringList();
    for (int i = 1; i < argc; ++i) {
        result.remainingArgs.append(QString::fromUtf8(argv[i]));
    }

    if (!knownCommands().contains(result.command)) {
        result.errorMessage = "Unknown command: " + result.command +
                              ". Available: " + knownCommands().join(", ");
        return result;
    }

    result.valid = true;
    return result;
}

QStringList MainParser::knownCommands() {
    return QStringList()
        << "library"
        << "proton"
        << "prefix"
        << "env"
        << "preset";
}

// ============================================================================
// LibraryParser
// ============================================================================

ParserResult LibraryParser::parse(int argc, const char *const *argv, void *out) {
    ParserResult result;
    Q_UNUSED(out);

    if (argc < 1) {
        result.errorMessage = "Missing subcommand. Available: list, search, add, launch, remove";
        printUsage("library", "<subcommand> [args...]");
        return result;
    }

    result.subcommand = QString::fromUtf8(argv[0]);
    result.command = "library";
    result.remainingArgs = QStringList();
    for (int i = 1; i < argc; ++i) {
        result.remainingArgs.append(QString::fromUtf8(argv[i]));
    }

    if (result.subcommand == "list") {
        result.valid = true;
        // No typed data needed for list
        return result;

    } else if (result.subcommand == "search") {
        if (argc < 2) {
            result.errorMessage = "search requires a query argument";
            printUsage("library search", "<query>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "add") {
        if (argc < 3) {
            result.errorMessage = "add requires <name> and <executable> arguments";
            printUsage("library add", "<name> <executable> [--proton <version>] [--prefix-mode <mode>]");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "launch") {
        if (argc < 1) {
            result.errorMessage = "launch requires a game identifier (name or UUID)";
            printUsage("library launch", "<game-name-or-id>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "remove") {
        if (argc < 1) {
            result.errorMessage = "remove requires a game ID";
            printUsage("library remove", "<game-id>");
            return result;
        }
        result.valid = true;
        return result;

    } else {
        result.errorMessage = "Unknown subcommand: " + result.subcommand;
        printError(result.errorMessage);
        qDebug() << "Available: list, search, add, launch, remove";
        return result;
    }
}

QList<LibraryListArgs> LibraryParser::parseList(int argc, const char *const *argv) {
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    return QList<LibraryListArgs>{};
}

LibrarySearchArgs LibraryParser::parseSearch(int argc, const char *const *argv, bool &ok) {
    LibrarySearchArgs args;
    ok = false;

    if (argc < 2) {
        printError("search requires a query argument");
        printUsage("library search", "<query>");
        return args;
    }

    args.query = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

LibraryAddArgs LibraryParser::parseAdd(int argc, const char *const *argv, bool &ok) {
    LibraryAddArgs args;
    ok = false;

    if (argc < 3) {
        printError("add requires <name> and <executable> arguments");
        printUsage("library add", "<name> <executable> [--proton <version>] [--prefix-mode <mode>]");
        return args;
    }

    args.name = QString::fromUtf8(argv[1]);
    args.executable = QString::fromUtf8(argv[2]);
    args.prefixMode = "per-game";

    // Parse optional flags
    for (int i = 3; i < argc; ++i) {
        QString opt = QString::fromUtf8(argv[i]);
        if (opt == "--proton" && i + 1 < argc) {
            args.protonVersion = QString::fromUtf8(argv[++i]);
        } else if (opt == "--prefix-mode" && i + 1 < argc) {
            args.prefixMode = QString::fromUtf8(argv[++i]);
            if (args.prefixMode == "shared" && i + 1 < argc) {
                QString next = QString::fromUtf8(argv[i + 1]);
                if (!next.startsWith("--")) {
                    // Could be shared prefix ID — but we need --shared-prefix flag for clarity
                }
            }
        } else if (opt == "--shared-prefix" && i + 1 < argc) {
            args.sharedPrefixId = QString::fromUtf8(argv[++i]);
        } else if (opt == "--args" && i + 1 < argc) {
            args.launchArgs.append(QString::fromUtf8(argv[++i]));
        }
    }

    ok = true;
    return args;
}

LibraryLaunchArgs LibraryParser::parseLaunch(int argc, const char *const *argv, bool &ok) {
    LibraryLaunchArgs args;
    ok = false;

    if (argc < 2) {
        printError("launch requires a game identifier (name or UUID)");
        printUsage("library launch", "<game-name-or-id>");
        return args;
    }

    args.identifier = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

LibraryRemoveArgs LibraryParser::parseRemove(int argc, const char *const *argv, bool &ok) {
    LibraryRemoveArgs args;
    ok = false;

    if (argc < 2) {
        printError("remove requires a game ID");
        printUsage("library remove", "<game-id>");
        return args;
    }

    args.id = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

// ============================================================================
// ProtonParser
// ============================================================================

ParserResult ProtonParser::parse(int argc, const char *const *argv, void *out) {
    ParserResult result;
    Q_UNUSED(out);

    if (argc < 1) {
        result.errorMessage = "Missing subcommand. Available: list, suggest, set, download, delete";
        printUsage("proton", "<subcommand> [args...]");
        return result;
    }

    result.subcommand = QString::fromUtf8(argv[0]);
    result.command = "proton";
    result.remainingArgs = QStringList();
    for (int i = 1; i < argc; ++i) {
        result.remainingArgs.append(QString::fromUtf8(argv[i]));
    }

    if (result.subcommand == "list") {
        result.valid = true;
        return result;

    } else if (result.subcommand == "suggest") {
        if (argc < 2) {
            result.errorMessage = "suggest requires a game name";
            printUsage("proton suggest", "<game-name>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "set") {
        if (argc < 3) {
            result.errorMessage = "set requires <game-name> and <proton-version>";
            printUsage("proton set", "<game-name> <proton-version>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "download") {
        if (argc < 2) {
            result.errorMessage = "download requires a version";
            printUsage("proton download", "<version>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "delete") {
        if (argc < 2) {
            result.errorMessage = "delete requires a version";
            printUsage("proton delete", "<version>");
            return result;
        }
        result.valid = true;
        return result;

    } else {
        result.errorMessage = "Unknown subcommand: " + result.subcommand;
        printError(result.errorMessage);
        qDebug() << "Available: list, suggest, set, download, delete";
        return result;
    }
}

ProtonSuggestArgs ProtonParser::parseSuggest(int argc, const char *const *argv, bool &ok) {
    ProtonSuggestArgs args;
    ok = false;

    if (argc < 2) {
        printError("suggest requires a game name");
        printUsage("proton suggest", "<game-name>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

ProtonSetArgs ProtonParser::parseSet(int argc, const char *const *argv, bool &ok) {
    ProtonSetArgs args;
    ok = false;

    if (argc < 3) {
        printError("set requires <game-name> and <proton-version>");
        printUsage("proton set", "<game-name> <proton-version>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    args.version = QString::fromUtf8(argv[2]);
    ok = true;
    return args;
}

ProtonDownloadArgs ProtonParser::parseDownload(int argc, const char *const *argv, bool &ok) {
    ProtonDownloadArgs args;
    ok = false;

    if (argc < 2) {
        printError("download requires a version");
        printUsage("proton download", "<version>");
        return args;
    }

    args.version = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

ProtonDeleteArgs ProtonParser::parseDelete(int argc, const char *const *argv, bool &ok) {
    ProtonDeleteArgs args;
    ok = false;

    if (argc < 2) {
        printError("delete requires a version");
        printUsage("proton delete", "<version>");
        return args;
    }

    args.version = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

// ============================================================================
// PrefixParser
// ============================================================================

ParserResult PrefixParser::parse(int argc, const char *const *argv, void *out) {
    ParserResult result;
    Q_UNUSED(out);

    if (argc < 1) {
        result.errorMessage = "Missing subcommand. Available: open, reset, run, shared";
        printUsage("prefix", "<subcommand> [args...]");
        return result;
    }

    result.subcommand = QString::fromUtf8(argv[0]);
    result.command = "prefix";
    result.remainingArgs = QStringList();
    for (int i = 1; i < argc; ++i) {
        result.remainingArgs.append(QString::fromUtf8(argv[i]));
    }

    if (result.subcommand == "open") {
        if (argc < 2) {
            result.errorMessage = "open requires a game name";
            printUsage("prefix open", "<game-name>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "reset") {
        if (argc < 2) {
            result.errorMessage = "reset requires a game name";
            printUsage("prefix reset", "<game-name>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "run") {
        if (argc < 3) {
            result.errorMessage = "run requires <game-name> and <command>";
            printUsage("prefix run", "<game-name> <command> [args...]");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "shared") {
        if (argc < 2) {
            result.errorMessage = "shared requires a subcommand: list, create, assign, run";
            printUsage("prefix shared", "<list|create|assign|run> [args...]");
            return result;
        }
        // Delegate to shared sub-parsers
        QString sharedSub = QString::fromUtf8(argv[1]);
        result.remainingArgs.clear();
        for (int i = 2; i < argc; ++i) {
            result.remainingArgs.append(QString::fromUtf8(argv[i]));
        }

        if (sharedSub == "list") {
            result.subcommand = "shared-list";
            result.valid = true;
            return result;
        } else if (sharedSub == "create") {
            if (argc < 4) {
                result.errorMessage = "shared create requires <id> and <name>";
                printUsage("prefix shared create", "<id> <name> [--proton <version>]");
                return result;
            }
            result.subcommand = "shared-create";
            result.valid = true;
            return result;
        } else if (sharedSub == "assign") {
            if (argc < 4) {
                result.errorMessage = "shared assign requires <game-name> and <prefix-id>";
                printUsage("prefix shared assign", "<game-name> <prefix-id>");
                return result;
            }
            result.subcommand = "shared-assign";
            result.valid = true;
            return result;
        } else if (sharedSub == "run") {
            if (argc < 4) {
                result.errorMessage = "shared run requires <prefix-id> and <command>";
                printUsage("prefix shared run", "<prefix-id> <command> [args...]");
                return result;
            }
            result.subcommand = "shared-run";
            result.valid = true;
            return result;
        } else {
            result.errorMessage = "Unknown shared subcommand: " + sharedSub;
            printError(result.errorMessage);
            qDebug() << "Available: list, create, assign, run";
            return result;
        }

    } else {
        result.errorMessage = "Unknown subcommand: " + result.subcommand;
        printError(result.errorMessage);
        qDebug() << "Available: open, reset, run, shared";
        return result;
    }
}

PrefixOpenArgs PrefixParser::parseOpen(int argc, const char *const *argv, bool &ok) {
    PrefixOpenArgs args;
    ok = false;

    if (argc < 2) {
        printError("open requires a game name");
        printUsage("prefix open", "<game-name>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

PrefixResetArgs PrefixParser::parseReset(int argc, const char *const *argv, bool &ok) {
    PrefixResetArgs args;
    ok = false;

    if (argc < 2) {
        printError("reset requires a game name");
        printUsage("prefix reset", "<game-name>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

PrefixRunArgs PrefixParser::parseRun(int argc, const char *const *argv, bool &ok) {
    PrefixRunArgs args;
    ok = false;

    if (argc < 3) {
        printError("run requires <game-name> and <command>");
        printUsage("prefix run", "<game-name> <command> [args...]");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    args.command = QString::fromUtf8(argv[2]);
    for (int i = 3; i < argc; ++i) {
        args.args.append(QString::fromUtf8(argv[i]));
    }
    ok = true;
    return args;
}

PrefixSharedListArgs PrefixParser::parseSharedList(int argc, const char *const *argv, bool &ok) {
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    ok = true;
    return PrefixSharedListArgs{};
}

PrefixSharedCreateArgs PrefixParser::parseSharedCreate(int argc, const char *const *argv, bool &ok) {
    PrefixSharedCreateArgs args;
    ok = false;

    if (argc < 4) {
        printError("shared create requires <id> and <name>");
        printUsage("prefix shared create", "<id> <name> [--proton <version>]");
        return args;
    }

    args.id = QString::fromUtf8(argv[1]);
    args.name = QString::fromUtf8(argv[2]);
    args.protonVersion = QString::fromUtf8(argv[3]);

    // Parse optional flags
    for (int i = 4; i < argc; ++i) {
        QString opt = QString::fromUtf8(argv[i]);
        if (opt == "--proton" && i + 1 < argc) {
            args.protonVersion = QString::fromUtf8(argv[++i]);
        }
    }
    ok = true;
    return args;
}

PrefixSharedAssignArgs PrefixParser::parseSharedAssign(int argc, const char *const *argv, bool &ok) {
    PrefixSharedAssignArgs args;
    ok = false;

    if (argc < 4) {
        printError("shared assign requires <game-name> and <prefix-id>");
        printUsage("prefix shared assign", "<game-name> <prefix-id>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    args.prefixId = QString::fromUtf8(argv[2]);
    ok = true;
    return args;
}

PrefixSharedRunArgs PrefixParser::parseSharedRun(int argc, const char *const *argv, bool &ok) {
    PrefixSharedRunArgs args;
    ok = false;

    if (argc < 4) {
        printError("shared run requires <prefix-id> and <command>");
        printUsage("prefix shared run", "<prefix-id> <command> [args...]");
        return args;
    }

    args.prefixId = QString::fromUtf8(argv[1]);
    args.command = QString::fromUtf8(argv[2]);
    for (int i = 3; i < argc; ++i) {
        args.args.append(QString::fromUtf8(argv[i]));
    }
    ok = true;
    return args;
}

// ============================================================================
// EnvParser
// ============================================================================

ParserResult EnvParser::parse(int argc, const char *const *argv, void *out) {
    ParserResult result;
    Q_UNUSED(out);

    if (argc < 1) {
        result.errorMessage = "Missing subcommand. Available: set, list, get, validate";
        printUsage("env", "<subcommand> [args...]");
        return result;
    }

    result.subcommand = QString::fromUtf8(argv[0]);
    result.command = "env";
    result.remainingArgs = QStringList();
    for (int i = 1; i < argc; ++i) {
        result.remainingArgs.append(QString::fromUtf8(argv[i]));
    }

    if (result.subcommand == "set") {
        if (argc < 4) {
            result.errorMessage = "set requires <game-name> <key> <value>";
            printUsage("env set", "<game-name> <key> <value>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "list") {
        if (argc < 2) {
            result.errorMessage = "list requires a game name";
            printUsage("env list", "<game-name>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "get") {
        if (argc < 3) {
            result.errorMessage = "get requires <game-name> and <key>";
            printUsage("env get", "<game-name> <key>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "validate") {
        if (argc < 3) {
            result.errorMessage = "validate requires <key> and <value>";
            printUsage("env validate", "<key> <value>");
            return result;
        }
        result.valid = true;
        return result;

    } else {
        result.errorMessage = "Unknown subcommand: " + result.subcommand;
        printError(result.errorMessage);
        qDebug() << "Available: set, list, get, validate";
        return result;
    }
}

EnvSetArgs EnvParser::parseSet(int argc, const char *const *argv, bool &ok) {
    EnvSetArgs args;
    ok = false;

    if (argc < 4) {
        printError("set requires <game-name> <key> <value>");
        printUsage("env set", "<game-name> <key> <value>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    args.key = QString::fromUtf8(argv[2]);
    args.value = QString::fromUtf8(argv[3]);
    ok = true;
    return args;
}

EnvListArgs EnvParser::parseList(int argc, const char *const *argv, bool &ok) {
    EnvListArgs args;
    ok = false;

    if (argc < 2) {
        printError("list requires a game name");
        printUsage("env list", "<game-name>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

EnvGetArgs EnvParser::parseGet(int argc, const char *const *argv, bool &ok) {
    EnvGetArgs args;
    ok = false;

    if (argc < 3) {
        printError("get requires <game-name> and <key>");
        printUsage("env get", "<game-name> <key>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    args.key = QString::fromUtf8(argv[2]);
    ok = true;
    return args;
}

EnvValidateArgs EnvParser::parseValidate(int argc, const char *const *argv, bool &ok) {
    EnvValidateArgs args;
    ok = false;

    if (argc < 3) {
        printError("validate requires <key> and <value>");
        printUsage("env validate", "<key> <value>");
        return args;
    }

    args.key = QString::fromUtf8(argv[1]);
    args.value = QString::fromUtf8(argv[2]);
    ok = true;
    return args;
}

// ============================================================================
// PresetParser
// ============================================================================

ParserResult PresetParser::parse(int argc, const char *const *argv, void *out) {
    ParserResult result;
    Q_UNUSED(out);

    if (argc < 1) {
        result.errorMessage = "Missing subcommand. Available: list, apply, save, delete, refresh";
        printUsage("preset", "<subcommand> [args...]");
        return result;
    }

    result.subcommand = QString::fromUtf8(argv[0]);
    result.command = "preset";
    result.remainingArgs = QStringList();
    for (int i = 1; i < argc; ++i) {
        result.remainingArgs.append(QString::fromUtf8(argv[i]));
    }

    if (result.subcommand == "list") {
        if (argc < 2) {
            result.errorMessage = "list requires a game name";
            printUsage("preset list", "<game-name>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "apply") {
        if (argc < 3) {
            result.errorMessage = "apply requires <game-name> and <preset-id>";
            printUsage("preset apply", "<game-name> <preset-id>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "save") {
        if (argc < 2) {
            result.errorMessage = "save requires <game-name>";
            printUsage("preset save", "<game-name> [label]");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "delete") {
        if (argc < 3) {
            result.errorMessage = "delete requires <game-name> and <preset-id>";
            printUsage("preset delete", "<game-name> <preset-id>");
            return result;
        }
        result.valid = true;
        return result;

    } else if (result.subcommand == "refresh") {
        result.valid = true;
        return result;

    } else {
        result.errorMessage = "Unknown subcommand: " + result.subcommand;
        printError(result.errorMessage);
        qDebug() << "Available: list, apply, save, delete, refresh";
        return result;
    }
}

PresetListArgs PresetParser::parseList(int argc, const char *const *argv, bool &ok) {
    PresetListArgs args;
    ok = false;

    if (argc < 2) {
        printError("list requires a game name");
        printUsage("preset list", "<game-name>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    ok = true;
    return args;
}

PresetApplyArgs PresetParser::parseApply(int argc, const char *const *argv, bool &ok) {
    PresetApplyArgs args;
    ok = false;

    if (argc < 3) {
        printError("apply requires <game-name> and <preset-id>");
        printUsage("preset apply", "<game-name> <preset-id>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    args.presetId = QString::fromUtf8(argv[2]);
    ok = true;
    return args;
}

PresetSaveArgs PresetParser::parseSave(int argc, const char *const *argv, bool &ok) {
    PresetSaveArgs args;
    ok = false;

    if (argc < 2) {
        printError("save requires <game-name>");
        printUsage("preset save", "<game-name> [label]");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    args.label = argc > 2 ? QString::fromUtf8(argv[2]) : "Custom";
    ok = true;
    return args;
}

PresetDeleteArgs PresetParser::parseDelete(int argc, const char *const *argv, bool &ok) {
    PresetDeleteArgs args;
    ok = false;

    if (argc < 3) {
        printError("delete requires <game-name> and <preset-id>");
        printUsage("preset delete", "<game-name> <preset-id>");
        return args;
    }

    args.gameName = QString::fromUtf8(argv[1]);
    args.presetId = QString::fromUtf8(argv[2]);
    ok = true;
    return args;
}

// ============================================================================
// Command table & dispatch
// ============================================================================

// Forward declarations — these live in the command files
int cmd_library(int argc, const char *const *argv);
int cmd_proton(int argc, const char *const *argv);
int cmd_prefix(int argc, const char *const *argv);
int cmd_env(int argc, const char *const *argv);
int cmd_preset(int argc, const char *const *argv);

static const QList<CommandEntry> g_commandTable = {
    {
        "library",
        {"list", "search", "add", "launch", "remove"},
        cmd_library
    },
    {
        "proton",
        {"list", "suggest", "set", "download", "delete"},
        cmd_proton
    },
    {
        "prefix",
        {"open", "reset", "run", "shared"},
        cmd_prefix
    },
    {
        "env",
        {"set", "list", "get", "validate"},
        cmd_env
    },
    {
        "preset",
        {"list", "apply", "save", "delete", "refresh"},
        cmd_preset
    }
};

const CommandEntry *findCommand(const QString &name) {
    for (const auto &entry : g_commandTable) {
        if (entry.name == name) {
            return &entry;
        }
    }
    return nullptr;
}

const QList<CommandEntry> &commandTable() {
    return g_commandTable;
}
