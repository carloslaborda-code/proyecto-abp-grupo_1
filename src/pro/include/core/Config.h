#pragma once

#include <string>

enum class GameMode {
    Story,
    Challenge,
    Infinite
};

class Config {
public:
    static Config& getInstance();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    unsigned int windowWidth() const;
    unsigned int windowHeight() const;
    unsigned int targetFps() const;
    float hudReservedPixels() const;
    const std::string& windowTitle() const;

    GameMode selectedMode() const;
    void setSelectedMode(GameMode mode);

    float masterVolume() const;
    void setMasterVolume(float volume);
    void changeMasterVolume(float delta);

    int windowPresetIndex() const;
    void cycleWindowPreset(int direction);
    std::string windowPresetLabel() const;

    static const char* gameModeLabel(GameMode mode);

private:
    Config() = default;

    unsigned int targetFps_ = 60;
    float hudReservedPixels_ = 140.f;
    std::string windowTitle_ = "Binary Assault";
    GameMode selectedMode_ = GameMode::Story;
    float masterVolume_ = 55.f;
    int windowPresetIndex_ = 0;
};
