#include "orbital/presetmanager.h"
#include "orbital/paths.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcPresetManager, "orbital.presetmanager")

namespace Orbital {

PresetManager::PresetManager(QObject *parent) : QObject(parent) {}

PresetManager::~PresetManager() = default;

void PresetManager::init() {
    // Load preset index
    qCInfo(lcPresetManager) << "PresetManager initialized";
}

QList<Preset> PresetManager::presetsForGame(const QString &gameName) const {
    Q_UNUSED(gameName);
    return {};
}

void PresetManager::refresh() {
    qCInfo(lcPresetManager) << "Refreshing preset cache";
}

void PresetManager::deleteLocalPreset(const QString &gameName, const QString &presetId) {
    Q_UNUSED(gameName);
    Q_UNUSED(presetId);
    qCInfo(lcPresetManager) << "Delete preset:" << presetId << "for" << gameName;
}

void PresetManager::onNetworkReply() {
    // TODO: Handle network reply
    qCDebug(lcPresetManager) << "Network reply handler (stub)";
}

} // namespace Orbital
