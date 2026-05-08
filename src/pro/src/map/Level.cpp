#include "map/Level.h"

bool Level::isValid() const {
    return !lanes.empty() && tileMap.getWidth() > 0 && tileMap.getHeight() > 0;
}

sf::Vector2f Level::laneSpawnCenter(int laneId) const {
    return grid.tileCenterToWorld(lanes[laneId].spawnTile.x, lanes[laneId].spawnTile.y);
}

sf::Vector2f Level::laneGoalCenter(int laneId) const {
    return grid.tileCenterToWorld(lanes[laneId].goalTile.x, lanes[laneId].goalTile.y);
}

sf::Vector2f Level::laneDefenseCenter(int laneId) const {
    return grid.tileCenterToWorld(lanes[laneId].defenseTile.x, lanes[laneId].defenseTile.y);
}

float Level::laneCenterY(int laneId) const {
    return laneGoalCenter(laneId).y;
}
