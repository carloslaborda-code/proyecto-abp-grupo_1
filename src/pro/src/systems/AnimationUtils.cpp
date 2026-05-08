#include "systems/AnimationUtils.h"

#include <algorithm>
#include <array>
#include <filesystem>

namespace {

namespace fs = std::filesystem;

float enemyVisualScaleForType(EnemyType type) {
    constexpr float kBaseScale = 0.75f;
    switch (type) {
        case EnemyType::Standard:
            return kBaseScale;
        case EnemyType::Fast:
            return kBaseScale * 1.25f;
        case EnemyType::Heavy:
            return kBaseScale * 1.75f;
        case EnemyType::Adaptive:
            return kBaseScale * 1.15f;
    }

    return kBaseScale;
}

bool assetExists(const std::string& relativePath) {
    const std::array<std::string, 4> candidates{
        relativePath,
        "resources/" + relativePath,
        "../resources/" + relativePath,
        "src/pro/BinaryAssault/resources/" + relativePath,
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return true;
        }
    }

    return false;
}

AnimationConfig empAnimationForState(EMPState state) {
    constexpr float kEmpPlacementScale = 0.55f;
    constexpr float kEmpArmedScale = 0.46f;
    constexpr float kEmpTriggeredScale = 0.44f;

    switch (state) {
        case EMPState::Placement:
            if (assetExists("sprites_defensas/fase1colocacionemp.png")) {
                return {"sprites_defensas/fase1colocacionemp.png", 1, 0.2f, true, kEmpPlacementScale, kEmpPlacementScale};
            }
            return {"sprites_defensas/Emp (1).png", 5, 0.2f, true, 1.0f, 1.0f};
        case EMPState::Armed:
            if (assetExists("sprites_defensas/fase2activacionaesperadeexplosion.png")) {
                return {"sprites_defensas/fase2activacionaesperadeexplosion.png", 1, 0.2f, true, kEmpArmedScale, kEmpArmedScale};
            }
            return {"sprites_defensas/Emp (1).png", 5, 0.2f, true, 1.0f, 1.0f};
        case EMPState::Triggered:
            if (assetExists("sprites_defensas/fase3explosionn.png")) {
                return {"sprites_defensas/fase3explosionn.png", 1, 0.2f, true, kEmpTriggeredScale, kEmpTriggeredScale};
            }
            return {"sprites_defensas/Emp (1).png", 5, 0.2f, true, 1.0f, 1.0f};
    }

    return {"sprites_defensas/Emp (1).png", 5, 0.2f, true, 1.0f, 1.0f};
}

}  // namespace

SpriteAnimation makeAnimation(TextureManager& resources, const AnimationConfig& config) {
    SpriteAnimation animation;
    animation.texture = resources.loadTexture(config.path);
    animation.frameCount = std::max(1, config.frameCount);
    animation.frameTime = config.frameTime;
    animation.verticalSheet = config.verticalSheet;
    animation.scaleX = config.scaleX;
    animation.scaleY = config.scaleY;

    if (!animation.texture) {
        return animation;
    }

    const sf::Vector2u textureSize = animation.texture->getSize();
    if (animation.verticalSheet) {
        animation.frameWidth = static_cast<int>(textureSize.x);
        animation.frameHeight = static_cast<int>(textureSize.y) / animation.frameCount;
    } else {
        animation.frameWidth = static_cast<int>(textureSize.x) / animation.frameCount;
        animation.frameHeight = static_cast<int>(textureSize.y);
    }

    animation.loaded = animation.frameWidth > 0 && animation.frameHeight > 0;
    return animation;
}

void updateAnimation(SpriteAnimation& animation, float dt) {
    if (!animation.loaded || animation.frameCount <= 1) {
        return;
    }

    animation.timer += dt;
    if (animation.timer < animation.frameTime) {
        return;
    }

    animation.timer = 0.f;
    animation.currentFrame = (animation.currentFrame + 1) % animation.frameCount;
}

void drawAnimation(sf::RenderWindow& window,
                   const SpriteAnimation& animation,
                   const sf::Vector2f& entityPos,
                   const sf::Vector2f& entitySize,
                   bool flipX,
                   bool alignBottom,
                   const sf::Color& tint) {
    if (!animation.loaded || !animation.texture) {
        return;
    }

    sf::Sprite sprite(*animation.texture);

    const int left = animation.verticalSheet ? 0 : animation.currentFrame * animation.frameWidth;
    const int top = animation.verticalSheet ? animation.currentFrame * animation.frameHeight : 0;
    sprite.setTextureRect(sf::IntRect(left, top, animation.frameWidth, animation.frameHeight));
    sprite.setColor(tint);

    const float scaledWidth = animation.frameWidth * animation.scaleX;
    const float scaledHeight = animation.frameHeight * animation.scaleY;
    const float drawX = entityPos.x - (scaledWidth - entitySize.x) / 2.f;
    const float drawY = alignBottom
                            ? entityPos.y + entitySize.y - scaledHeight
                            : entityPos.y - (scaledHeight - entitySize.y) / 2.f;

    if (flipX) {
        sprite.setOrigin(static_cast<float>(animation.frameWidth), 0.f);
        sprite.setScale(-animation.scaleX, animation.scaleY);
        sprite.setPosition(drawX, drawY);
    } else {
        sprite.setScale(animation.scaleX, animation.scaleY);
        sprite.setPosition(drawX, drawY);
    }

    window.draw(sprite);
}

AnimationConfig defenseAnimationForType(DefenseType type) {
    switch (type) {
        case DefenseType::Firewall:
            return {"sprites_defensas/FireWall(Torreta).png", 3, 0.18f, true, 1.5f, 1.5f};
        case DefenseType::EMP:
            return empAnimationForState(EMPState::Armed);
        case DefenseType::AuxiliaryServer:
            return {"sprites_defensas/Servidor auxiliar (1).png", 6, 0.18f, true, 1.0f, 1.0f};
        case DefenseType::SlowNode:
            return {"sprites_defensas/Realentizador (2).png", 2, 0.22f, true, 1.0f, 1.0f};
    }

    return {};
}

AnimationConfig defenseAnimationFor(const Defense& defense) {
    if (defense.type == DefenseType::EMP) {
        return empAnimationForState(defense.empState);
    }

    return defenseAnimationForType(defense.type);
}

AnimationConfig enemyMoveAnimationForType(EnemyType type) {
    const float visualScale = enemyVisualScaleForType(type);

    switch (type) {
        case EnemyType::Standard:
            return {"hoja-de-enemigos/enemigo-estandar-mov.png", 3, 0.18f, false, visualScale, visualScale};
        case EnemyType::Fast:
            return {"hoja-de-enemigos/personaje-veloz-mov.png", 3, 0.14f, false, visualScale, visualScale};
        case EnemyType::Heavy:
            return {"hoja-de-enemigos/enemigo-tanque-mov.png", 3, 0.22f, false, visualScale, visualScale};
        case EnemyType::Adaptive:
            return {"hoja-de-enemigos/enemigo-adaptativo-mov.png", 3, 0.16f, false, visualScale, visualScale};
    }

    return {};
}

AnimationConfig enemyAttackAnimationForType(EnemyType type) {
    const float visualScale = enemyVisualScaleForType(type);

    switch (type) {
        case EnemyType::Standard:
            return {"hoja-de-enemigos/enemigo-standar-shoot.png", 3, 0.18f, false, visualScale, visualScale};
        case EnemyType::Fast:
            return {"hoja-de-enemigos/enemigo-veloz-shoot.png", 3, 0.14f, false, visualScale, visualScale};
        case EnemyType::Heavy:
            return {"hoja-de-enemigos/enemigo-tanque-shoot.png", 3, 0.22f, false, visualScale, visualScale};
        case EnemyType::Adaptive:
            return {"hoja-de-enemigos/enemigo-adaptativo-shot.png", 3, 0.16f, false, visualScale, visualScale};
    }

    return {};
}
