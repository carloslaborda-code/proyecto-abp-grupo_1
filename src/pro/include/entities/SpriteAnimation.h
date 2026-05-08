#pragma once

#include <memory>

#include <SFML/Graphics.hpp>

struct SpriteAnimation {
    std::shared_ptr<sf::Texture> texture;
    int frameWidth = 0;
    int frameHeight = 0;
    int frameCount = 1;
    int currentFrame = 0;
    float frameTime = 0.2f;
    float timer = 0.f;
    bool verticalSheet = false;
    bool loaded = false;
    float scaleX = 1.f;
    float scaleY = 1.f;
};
