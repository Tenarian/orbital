#include "orbital/prefixmanager.h"
#include "orbital/paths.h"
#include <QDir>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcPrefixManager, "orbital.prefixmanager")

namespace Orbital {

PrefixManager::PrefixManager(QObject *parent) : QObject(parent) {}

PrefixManager::~PrefixManager() = default;

QString PrefixManager::prefixPath(const QString &gameId) const {
    Q_UNUSED(gameId);
    return Paths::prefixes() + "/" + gameId;
}

void PrefixManager::resetPrefix(const QString &gameId) {
    Q_UNUSED(gameId);
    // TODO: Backup and remove prefix
    qCInfo(lcPrefixManager) << "Reset prefix requested:" << gameId;
}

QList<PrefixMeta> PrefixManager::listSharedPrefixes() const {
    QList<PrefixMeta> result;
    QDir dir(Paths::prefixesShared());
    if (dir.exists()) {
        for (const auto &entry : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            PrefixMeta meta;
            meta.id = entry.fileName();
            meta.name = meta.id;
            result.append(meta);
        }
    }
    return result;
}

} // namespace Orbital
