#pragma once

#include <string>

#include <SFML/Graphics.hpp>

#include "entities/Defense.h"
#include "entities/Enemy.h"
#include "entities/SpriteAnimation.h"
#include "systems/TextureManager.h"

struct AnimationConfig {
    std::string path;
    int frameCount = 1;
    float frameTime = 0.2f;
    bool verticalSheet = false;
    float scaleX = 1.f;
    float scaleY = 1.f;
};

SpriteAnimation makeAnimation(TextureManager& resources, const AnimationConfig& config);
void updateAnimation(SpriteAnimation& animation, float dt);
void drawAnimation(sf::RenderWindow& window,
                   const SpriteAnimation& animation,
                   const sf::Vector2f& entityPos,
                   const sf::Vector2f& entitySize,
                   bool flipX,
                   bool alignBottom = false,
                   const sf::Color& tint = sf::Color::White);

AnimationConfig defenseAnimationForType(DefenseType type);
AnimationConfig defenseAnimationFor(const Defense& defense);
AnimationConfig enemyMoveAnimationForType(EnemyType type);
AnimationConfig enemyAttackAnimationForType(EnemyType type);
