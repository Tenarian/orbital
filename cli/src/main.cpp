#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "parser.h"

#include "orbital/gamelibrary.h"
#include "orbital/paths.h"
#include "orbital/processlauncher.h"
#include "orbital/protonmanager.h"
#include "orbital/prefixmanager.h"
#include "orbital/envvarschema.h"
#include "orbital/presetmanager.h"

// Forward declarations for commands
int cmd_library(int argc, const char *const *argv);
int cmd_proton(int argc, const char *const *argv);
int cmd_prefix(int argc, const char *const *argv);
int cmd_env(int argc, const char *const *argv);
int cmd_preset(int argc, const char *const *argv);

int main(int argc, const char *const *argv) {
    QCoreApplication app(argc, const_cast<char **>(argv));
    app.setApplicationName("orbital");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("orbital");
    app.setOrganizationDomain("orbital.dev");

    // Skip argv[0] (program name) — pass only user-supplied arguments
    if (argc < 2) {
        printError("Missing command. Available: " + MainParser::knownCommands().join(", "));
        printUsage("", "<command> [args...]");
        return 1;
    }

    // Parse and validate the top-level command
    auto mainResult = MainParser::parse(argc - 1, argv + 1);

    if (!mainResult.valid) {
        printError(mainResult.errorMessage);
        printUsage("", "<command> [args...]");
        qDebug() << "Available commands:" << MainParser::knownCommands().join(", ");
        return 1;
    }

    // Find the command entry
    const auto *cmdEntry = findCommand(mainResult.command);
    if (!cmdEntry) {
        printError("Command not found:" + mainResult.command);
        return 1;
    }

    // Initialize paths
    qDebug() << "Config dir:" << Orbital::Paths::config();
    qDebug() << "Data dir:" << Orbital::Paths::data();
    qDebug() << "Cache dir:" << Orbital::Paths::cache();

    // Build a stable argv array for the remaining args.
    // We keep the QByteArray buffers alive in a vector so the
    // const char* pointers remain valid during the handler call.
    QStringList remaining = mainResult.remainingArgs;
    int remainingCount = remaining.size();

    if (remainingCount == 0) {
        // No remaining args — call handler with nullptr
        int result = cmdEntry->handler(0, nullptr);
        return result;
    }

    // Store UTF-8 bytes so pointers stay valid
    QVector<QByteArray> buffers;
    buffers.reserve(remainingCount);
    QVector<const char *> pointers;
    pointers.reserve(remainingCount);

    for (const auto &str : remaining) {
        buffers.append(str.toUtf8());
        pointers.append(buffers.last().constData());
    }

    int result = cmdEntry->handler(remainingCount, pointers.constData());
    return result;
}
