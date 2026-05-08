#pragma once

#include <vector>

#include "map/Grid.h"
#include "map/Lane.h"
#include "map/TileMap.h"

class Level {
public:
    TileMap tileMap;
    Grid grid;
    std::vector<Lane> lanes;

    bool isValid() const;
    sf::Vector2f laneSpawnCenter(int laneId) const;
    sf::Vector2f laneGoalCenter(int laneId) const;
    sf::Vector2f laneDefenseCenter(int laneId) const;
    float laneCenterY(int laneId) const;
};
