#pragma once

#include <SFML/Graphics/Rect.hpp>

#include "entities/Entity.h"
#include "entities/SpriteAnimation.h"

enum class DefenseType {
    Firewall,
    EMP,
    AuxiliaryServer,
    SlowNode
};

enum class EMPState {
    Placement,
    Armed,
    Triggered
};

class Defense : public Entity {
public:
    Defense();

    AABB getAABB() const override;

    int laneId;
    DefenseType type;
    float hp;
    float maxHp;
    bool placed;
    int targetEnemyId;
    float fireTimer;
    float fireInterval;
    int damage;
    EMPState empState;
    float empPhaseTimer;
    sf::FloatRect slotBounds;
    SpriteAnimation animation;
};
