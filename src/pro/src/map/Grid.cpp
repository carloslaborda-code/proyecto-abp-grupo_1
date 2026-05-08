#include "map/Grid.h"

Grid::Grid()
    : width(0), height(0), tileWidth(0), tileHeight(0) {
}

Grid::Grid(int gridWidth, int gridHeight, int gridTileWidth, int gridTileHeight)
    : width(gridWidth),
      height(gridHeight),
      tileWidth(gridTileWidth),
      tileHeight(gridTileHeight) {
}

void Grid::setDimensions(int gridWidth, int gridHeight, int gridTileWidth, int gridTileHeight) {
    width = gridWidth;
    height = gridHeight;
    tileWidth = gridTileWidth;
    tileHeight = gridTileHeight;
}

void Grid::setCells(const std::vector<Cell>& newCells) {
    cells = newCells;
}

int Grid::getWidth() const {
    return width;
}

int Grid::getHeight() const {
    return height;
}

int Grid::getTileWidth() const {
    return tileWidth;
}

int Grid::getTileHeight() const {
    return tileHeight;
}

const Cell* Grid::getCell(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return nullptr;
    }

    const int index = y * width + x;
    if (index < 0 || index >= static_cast<int>(cells.size())) {
        return nullptr;
    }

    return &cells[index];
}

sf::Vector2f Grid::tileToWorld(int tileX, int tileY) const {
    return {static_cast<float>(tileX * tileWidth), static_cast<float>(tileY * tileHeight)};
}

sf::Vector2f Grid::tileCenterToWorld(int tileX, int tileY) const {
    return {static_cast<float>(tileX * tileWidth + tileWidth / 2),
            static_cast<float>(tileY * tileHeight + tileHeight / 2)};
}
