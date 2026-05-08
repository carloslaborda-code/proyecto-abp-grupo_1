#pragma once

#include <vector>

#include "entities/Projectile.h"

class ProjectileSystem {
public:
    static void removeInactive(std::vector<Projectile>& projectiles);
    static void snapshot(std::vector<Projectile>& projectiles);
    static void update(std::vector<Projectile>& projectiles, float dt, float worldWidth, float cullMargin = 80.f);
};
