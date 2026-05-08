#include "core/Config.h"

#include <algorithm>
#include <array>

namespace {

struct WindowPreset {
    unsigned int width;
    unsigned int height;
    const char* label;
};

const std::array<WindowPreset, 3>& windowPresets() {
    static const std::array<WindowPreset, 3> presets{{
        {1280, 720, "1280x720"},
        {1600, 900, "1600x900"},
        {1920, 1080, "1920x1080"},
    }};
    return presets;
}

}  // namespace

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

unsigned int Config::windowWidth() const {
    return windowPresets()[windowPresetIndex_].width;
}

unsigned int Config::windowHeight() const {
    return windowPresets()[windowPresetIndex_].height;
}

unsigned int Config::targetFps() const {
    return targetFps_;
}

float Config::hudReservedPixels() const {
    return hudReservedPixels_;
}

const std::string& Config::windowTitle() const {
    return windowTitle_;
}

GameMode Config::selectedMode() const {
    return selectedMode_;
}

void Config::setSelectedMode(GameMode mode) {
    selectedMode_ = mode;
}

float Config::masterVolume() const {
    return masterVolume_;
}

void Config::setMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.f, 100.f);
}

void Config::changeMasterVolume(float delta) {
    setMasterVolume(masterVolume_ + delta);
}

int Config::windowPresetIndex() const {
    return windowPresetIndex_;
}

void Config::cycleWindowPreset(int direction) {
    const int presetCount = static_cast<int>(windowPresets().size());
    if (presetCount <= 0) {
        return;
    }

    windowPresetIndex_ = (windowPresetIndex_ + direction) % presetCount;
    if (windowPresetIndex_ < 0) {
        windowPresetIndex_ += presetCount;
    }
}

std::string Config::windowPresetLabel() const {
    return windowPresets()[windowPresetIndex_].label;
}

const char* Config::gameModeLabel(GameMode mode) {
    switch (mode) {
        case GameMode::Story:
            return "Historia";
        case GameMode::Challenge:
            return "Desafio";
        case GameMode::Infinite:
            return "Infinito";
    }

    return "Historia";
}
