#include "entities/Projectile.h"

Projectile::Projectile()
    : Entity(Layer::Projectile),
      velocity(0.f, 0.f),
      damage(0),
      laneId(0),
      targetEnemyId(-1),
      remainingRange(0.f),
      owner(ProjectileOwner::Defense) {
}
