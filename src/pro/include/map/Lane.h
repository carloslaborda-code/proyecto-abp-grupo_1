#pragma once

#include <vector>

#include <SFML/System/Vector2.hpp>

struct Lane {
    int id = 0;
    sf::Vector2i spawnTile{0, 0};
    sf::Vector2i goalTile{0, 0};
    sf::Vector2i defenseTile{0, 0};
    std::vector<sf::Vector2i> buildTiles;
};
