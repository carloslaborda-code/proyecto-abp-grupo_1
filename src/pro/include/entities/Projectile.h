#pragma once

#include "entities/Entity.h"

enum class ProjectileOwner {
    Defense,
    Enemy
};

class Projectile : public Entity {
public:
    Projectile();

    sf::Vector2f velocity;
    int damage;
    int laneId;
    int targetEnemyId;
    float remainingRange;
    ProjectileOwner owner;
};
