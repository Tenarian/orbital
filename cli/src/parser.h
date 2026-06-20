#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>

#include "orbital/types.h"

// ============================================================================
// CLI helper functions (defined in parser.cpp, declared here for reuse)
// ============================================================================

void printUsage(const QString &command, const QString &help);
void printError(const QString &msg);
void printWarning(const QString &msg);

// ============================================================================
// ParserResult — generic result from any parser
// ============================================================================

struct ParserResult {
    bool valid = false;
    QString command;           // top-level command name
    QString subcommand;        // subcommand name (empty if none)
    QString errorMessage;      // set when valid == false
    QStringList remainingArgs; // args not consumed by the parser

    // Convenience
    bool operator==(const ParserResult &other) const {
        return valid == other.valid
            && command == other.command
            && subcommand == other.subcommand
            && errorMessage == other.errorMessage
            && remainingArgs == other.remainingArgs;
    }

    bool operator!=(const ParserResult &other) const { return !(*this == other); }
};

// ============================================================================
// MainParser — validates the top-level command
// ============================================================================

class MainParser {
public:
    // Parse argv[0..argc-1] as <command> [args...]
    // Returns the command name if recognised, or an error.
    static ParserResult parse(int argc, const char *const *argv);

    // Return the list of known top-level commands
    static QStringList knownCommands();
};

// ============================================================================
// LibraryParser — subcommands: list, search, add, launch, remove
// ============================================================================

struct LibraryListArgs {
    // No extra args needed
};

struct LibrarySearchArgs {
    QString query;
};

struct LibraryAddArgs {
    QString name;
    QString executable;
    QString protonVersion;
    QString prefixMode;       // "per-game" or "shared"
    QString sharedPrefixId;   // when prefixMode == "shared"
    QStringList launchArgs;
};

struct LibraryLaunchArgs {
    QString identifier;       // game name or UUID
};

struct LibraryRemoveArgs {
    QString id;               // game UUID
};

class LibraryParser {
public:
    // Parse <subcommand> [args...] into typed args.
    // On success, sets `out` and returns a valid ParserResult.
    // On failure, returns an error ParserResult (valid == false).
    static ParserResult parse(int argc, const char *const *argv, void *out);

    // Overload that returns the typed args directly (simpler for callers)
    static QList<LibraryListArgs> parseList(int argc, const char *const *argv);
    static LibrarySearchArgs parseSearch(int argc, const char *const *argv, bool &ok);
    static LibraryAddArgs parseAdd(int argc, const char *const *argv, bool &ok);
    static LibraryLaunchArgs parseLaunch(int argc, const char *const *argv, bool &ok);
    static LibraryRemoveArgs parseRemove(int argc, const char *const *argv, bool &ok);
};

// ============================================================================
// ProtonParser — subcommands: list, suggest, set, download, delete
// ============================================================================

struct ProtonSuggestArgs {
    QString gameName;
};

struct ProtonSetArgs {
    QString gameName;
    QString version;
};

struct ProtonDownloadArgs {
    QString version;
};

struct ProtonDeleteArgs {
    QString version;
};

class ProtonParser {
public:
    static ParserResult parse(int argc, const char *const *argv, void *out);

    static ProtonSuggestArgs parseSuggest(int argc, const char *const *argv, bool &ok);
    static ProtonSetArgs parseSet(int argc, const char *const *argv, bool &ok);
    static ProtonDownloadArgs parseDownload(int argc, const char *const *argv, bool &ok);
    static ProtonDeleteArgs parseDelete(int argc, const char *const *argv, bool &ok);
};

// ============================================================================
// PrefixParser — subcommands: open, reset, run, shared
// ============================================================================

struct PrefixOpenArgs {
    QString gameName;
};

struct PrefixResetArgs {
    QString gameName;
};

struct PrefixRunArgs {
    QString gameName;
    QString command;
    QStringList args;
};

struct PrefixSharedListArgs {
    // No args
};

struct PrefixSharedCreateArgs {
    QString id;
    QString name;
    QString protonVersion;
};

struct PrefixSharedAssignArgs {
    QString gameName;
    QString prefixId;
};

struct PrefixSharedRunArgs {
    QString prefixId;
    QString command;
    QStringList args;
};

class PrefixParser {
public:
    static ParserResult parse(int argc, const char *const *argv, void *out);

    static PrefixOpenArgs parseOpen(int argc, const char *const *argv, bool &ok);
    static PrefixResetArgs parseReset(int argc, const char *const *argv, bool &ok);
    static PrefixRunArgs parseRun(int argc, const char *const *argv, bool &ok);
    static PrefixSharedListArgs parseSharedList(int argc, const char *const *argv, bool &ok);
    static PrefixSharedCreateArgs parseSharedCreate(int argc, const char *const *argv, bool &ok);
    static PrefixSharedAssignArgs parseSharedAssign(int argc, const char *const *argv, bool &ok);
    static PrefixSharedRunArgs parseSharedRun(int argc, const char *const *argv, bool &ok);
};

// ============================================================================
// EnvParser — subcommands: set, list, get, validate
// ============================================================================

struct EnvSetArgs {
    QString gameName;
    QString key;
    QString value;
};

struct EnvListArgs {
    QString gameName;
};

struct EnvGetArgs {
    QString gameName;
    QString key;
};

struct EnvValidateArgs {
    QString key;
    QString value;
};

class EnvParser {
public:
    static ParserResult parse(int argc, const char *const *argv, void *out);

    static EnvSetArgs parseSet(int argc, const char *const *argv, bool &ok);
    static EnvListArgs parseList(int argc, const char *const *argv, bool &ok);
    static EnvGetArgs parseGet(int argc, const char *const *argv, bool &ok);
    static EnvValidateArgs parseValidate(int argc, const char *const *argv, bool &ok);
};

// ============================================================================
// PresetParser — subcommands: list, apply, save, delete, refresh
// ============================================================================

struct PresetListArgs {
    QString gameName;
};

struct PresetApplyArgs {
    QString gameName;
    QString presetId;
};

struct PresetSaveArgs {
    QString gameName;
    QString label;
};

struct PresetDeleteArgs {
    QString gameName;
    QString presetId;
};

class PresetParser {
public:
    static ParserResult parse(int argc, const char *const *argv, void *out);

    static PresetListArgs parseList(int argc, const char *const *argv, bool &ok);
    static PresetApplyArgs parseApply(int argc, const char *const *argv, bool &ok);
    static PresetSaveArgs parseSave(int argc, const char *const *argv, bool &ok);
    static PresetDeleteArgs parseDelete(int argc, const char *const *argv, bool &ok);
};

// ============================================================================
// Command dispatch — maps command name → parser + handler
// ============================================================================

// Each command handler receives the parsed typed args and returns an exit code.
// The actual command logic lives in the command files; these are just the
// parser entry points that the main.cpp dispatch layer calls.

using CommandHandler = int (*)(int argc, const char *const *argv);

// Register a command with its subcommand list and parser function.
struct CommandEntry {
    QString name;
    QStringList subcommands;
    CommandHandler handler;
};

// Return the full command table.
const CommandEntry *findCommand(const QString &name);
const QList<CommandEntry> &commandTable();
