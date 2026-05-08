#pragma once

#include "entities/Defense.h"
#include "map/Level.h"

class DefenseFactory {
public:
    static Defense createSlot(const Level& level, int laneId, const sf::Vector2i& tile);
    static void applyType(Defense& defense, DefenseType type, float empCooldown, float serverTickInterval);
};
