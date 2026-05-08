#include "entities/Defense.h"

Defense::Defense()
    : Entity(Layer::Defense),
      laneId(0),
      type(DefenseType::Firewall),
      hp(180.f),
      maxHp(180.f),
      placed(false),
      targetEnemyId(-1),
      fireTimer(0.f),
      fireInterval(0.65f),
      damage(25),
      empState(EMPState::Placement),
      empPhaseTimer(0.f),
      slotBounds() {
}

AABB Defense::getAABB() const {
    const float width = size.x * 0.68f;
    const float height = size.y * 0.80f;
    const float offsetX = (size.x - width) * 0.5f;
    const float offsetY = (size.y - height) * 0.5f;
    return {sf::FloatRect(position.x + offsetX, position.y + offsetY, width, height)};
}
