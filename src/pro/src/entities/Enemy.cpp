#include "entities/Enemy.h"

Enemy::Enemy()
    : Entity(Layer::Enemy),
      type(EnemyType::Standard),
      id(-1),
      baseSpeed(0.f),
      velocity(0.f, 0.f),
      hp(100),
      laneId(0),
      laneChangeTargetLaneId(-1),
      laneChangeTargetY(0.f),
      laneChangeCooldown(0.f),
      laneChangeUsed(false),
      blocked(false),
      slowed(false),
      pendingPushback(0.f),
      fireTimer(0.f),
      fireInterval(0.5f),
      projectileDamage(15),
      damageFlashTimer(0.f) {
}

AABB Enemy::getAABB() const {
    const float width = size.x * 0.54f;
    const float height = size.y * 0.80f;
    const float offsetX = (size.x - width) * 0.5f;
    const float offsetY = (size.y - height) * 0.5f;
    return {sf::FloatRect(position.x + offsetX, position.y + offsetY, width, height)};
}
