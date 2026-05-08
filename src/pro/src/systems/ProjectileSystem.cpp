#include "systems/ProjectileSystem.h"

#include <algorithm>
#include <cmath>

void ProjectileSystem::removeInactive(std::vector<Projectile>& projectiles) {
    projectiles.erase(
        std::remove_if(projectiles.begin(),
                       projectiles.end(),
                       [](const Projectile& projectile) { return !projectile.isAlive(); }),
        projectiles.end());
}

void ProjectileSystem::snapshot(std::vector<Projectile>& projectiles) {
    for (auto& projectile : projectiles) {
        projectile.snapshotPosition();
    }
}

void ProjectileSystem::update(std::vector<Projectile>& projectiles,
                              float dt,
                              float worldWidth,
                              float cullMargin) {
    const float rightCullX = worldWidth + cullMargin;
    const float leftCullX = -cullMargin;

    for (auto& projectile : projectiles) {
        if (!projectile.isAlive()) {
            continue;
        }

        projectile.setPosition(projectile.getPosition() + projectile.velocity * dt);
        projectile.remainingRange -= std::abs(projectile.velocity.x * dt);

        if (projectile.remainingRange <= 0.f ||
            projectile.getPosition().x > rightCullX ||
            projectile.getPosition().x + projectile.getSize().x < leftCullX) {
            projectile.setAlive(false);
        }
    }
}
