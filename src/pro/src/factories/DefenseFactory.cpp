#include "factories/DefenseFactory.h"

Defense DefenseFactory::createSlot(const Level& level, int laneId, const sf::Vector2i& tile) {
    Defense defense;
    defense.laneId = laneId;
    defense.type = DefenseType::Firewall;

    const float tileWidth = static_cast<float>(level.tileMap.getTileWidth());
    const float tileHeight = static_cast<float>(level.tileMap.getTileHeight());
    const sf::Vector2f tileOrigin = level.grid.tileToWorld(tile.x, tile.y);
    defense.slotBounds = sf::FloatRect(tileOrigin.x, tileOrigin.y, tileWidth, tileHeight);
    defense.setSize({tileWidth * 0.78f, tileHeight * 0.78f});

    const sf::Vector2f defenseCenter = level.grid.tileCenterToWorld(tile.x, tile.y);
    defense.setPosition(
        {defenseCenter.x - defense.getSize().x / 2.f, defenseCenter.y - defense.getSize().y / 2.f});
    defense.hp = 180.f;
    defense.maxHp = 180.f;
    defense.placed = false;
    defense.targetEnemyId = -1;
    defense.fireTimer = 0.f;
    defense.fireInterval = 0.75f;
    defense.damage = 20;
    defense.empState = EMPState::Placement;
    defense.empPhaseTimer = 0.f;
    defense.syncPosition();
    return defense;
}

void DefenseFactory::applyType(Defense& defense, DefenseType type, float empCooldown, float serverTickInterval) {
    defense.type = type;
    defense.fireTimer = 0.f;
    defense.targetEnemyId = -1;
    defense.maxHp = 175.f;
    defense.hp = defense.maxHp;
    defense.damage = 20;
    defense.fireInterval = 0.75f;
    defense.empState = EMPState::Placement;
    defense.empPhaseTimer = 0.f;

    if (type == DefenseType::EMP) {
        defense.damage = 9999;
        defense.fireInterval = empCooldown;
    } else if (type == DefenseType::AuxiliaryServer) {
        defense.damage = 0;
        defense.fireInterval = serverTickInterval;
    } else if (type == DefenseType::SlowNode) {
        defense.damage = 0;
        defense.fireInterval = 1.f;
    }
}
