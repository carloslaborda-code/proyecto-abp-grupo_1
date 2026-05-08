#pragma once

#include "entities/Entity.h"
#include "entities/SpriteAnimation.h"

enum class EnemyType {
    Standard,
    Fast,
    Heavy,
    Adaptive
};

class Enemy : public Entity {
public:
    Enemy();

    AABB getAABB() const override;

    EnemyType type;
    int id;
    float baseSpeed;
    sf::Vector2f velocity;
    int hp;
    int laneId;
    int laneChangeTargetLaneId;
    float laneChangeTargetY;
    float laneChangeCooldown;
    bool laneChangeUsed;
    bool blocked;
    bool slowed;
    float pendingPushback;
    float fireTimer;
    float fireInterval;
    int projectileDamage;
    float damageFlashTimer;
    SpriteAnimation moveAnimation;
    SpriteAnimation attackAnimation;
};
