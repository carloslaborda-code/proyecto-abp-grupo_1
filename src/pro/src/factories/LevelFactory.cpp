#include "factories/LevelFactory.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<std::string> mapCandidates() {
    return {
        "resources/carriles.tmx",
        "../resources/carriles.tmx",
        "src/pro/BinaryAssault/resources/carriles.tmx"
    };
}

}  // namespace

Level LevelFactory::createDefaultLevel() {
    Level level;

    bool loaded = false;
    for (const auto& candidate : mapCandidates()) {
        if (level.tileMap.loadFromTMX(candidate)) {
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        std::cerr << "No pude cargar el mapa TMX para BinaryAssault.\n";
        return level;
    }

    level.grid.setDimensions(
        level.tileMap.getWidth(),
        level.tileMap.getHeight(),
        level.tileMap.getTileWidth(),
        level.tileMap.getTileHeight());
    level.grid.setCells(level.tileMap.getCells());

    const auto& points = level.tileMap.getPoints();
    for (int index = 1;; ++index) {
        const auto spawnIt = points.find("Spawn" + std::to_string(index));
        const auto goalIt = points.find("Goal" + std::to_string(index));
        if (spawnIt == points.end() || goalIt == points.end()) {
            break;
        }

        Lane lane;
        lane.id = index - 1;

        if (spawnIt->second.x >= goalIt->second.x) {
            lane.spawnTile = spawnIt->second;
            lane.goalTile = goalIt->second;
        } else {
            lane.spawnTile = goalIt->second;
            lane.goalTile = spawnIt->second;
        }

        const int minX = std::min(lane.spawnTile.x, lane.goalTile.x);
        const int maxX = std::max(lane.spawnTile.x, lane.goalTile.x);
        const int defenseX = std::max(minX + 4, minX + (maxX - minX) / 2);
        lane.defenseTile = {defenseX, lane.goalTile.y};

        const int buildStartX = minX + 1;
        const int buildEndX = maxX - 1;
        for (int tileX = buildStartX; tileX <= buildEndX; ++tileX) {
            lane.buildTiles.push_back({tileX, lane.goalTile.y});
        }

        level.lanes.push_back(lane);
    }

    std::sort(level.lanes.begin(), level.lanes.end(), [&](const Lane& a, const Lane& b) {
        return level.grid.tileCenterToWorld(a.goalTile.x, a.goalTile.y).y <
               level.grid.tileCenterToWorld(b.goalTile.x, b.goalTile.y).y;
    });

    for (std::size_t i = 0; i < level.lanes.size(); ++i) {
        level.lanes[i].id = static_cast<int>(i);
    }

    return level;
}
