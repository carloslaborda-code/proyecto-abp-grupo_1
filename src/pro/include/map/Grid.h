#pragma once

#include <vector>

#include <SFML/System/Vector2.hpp>

#include "map/Cell.h"

class Grid {
public:
    Grid();
    Grid(int width, int height, int tileWidth, int tileHeight);

    void setDimensions(int width, int height, int tileWidth, int tileHeight);
    void setCells(const std::vector<Cell>& newCells);

    int getWidth() const;
    int getHeight() const;
    int getTileWidth() const;
    int getTileHeight() const;

    const Cell* getCell(int x, int y) const;
    sf::Vector2f tileToWorld(int tileX, int tileY) const;
    sf::Vector2f tileCenterToWorld(int tileX, int tileY) const;

private:
    int width;
    int height;
    int tileWidth;
    int tileHeight;
    std::vector<Cell> cells;
};
