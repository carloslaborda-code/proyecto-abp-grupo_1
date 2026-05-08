#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

#include "map/Cell.h"

struct TilesetData {
    int firstGid = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    int tileCount = 0;
    int columns = 0;
    std::string name;
    std::string imageSource;
    std::string resolvedImagePath;
    sf::Texture texture;
    bool loaded = false;
};

class TileMap {
public:
    bool loadFromTMX(const std::string& tmxPath);
    void draw(sf::RenderWindow& window) const;

    int getWidth() const;
    int getHeight() const;
    int getTileWidth() const;
    int getTileHeight() const;

    const std::vector<int>& getLogicGids() const;
    const std::vector<int>& getVisualGids() const;
    const std::vector<Cell>& getCells() const;
    const std::unordered_map<std::string, sf::Vector2i>& getPoints() const;

private:
    bool loadTextures();
    const TilesetData* findTilesetForGid(int gid) const;
    void drawFallback(sf::RenderWindow& window) const;

    int width = 0;
    int height = 0;
    int tileWidth = 0;
    int tileHeight = 0;

    std::vector<int> logicGids;
    std::vector<int> visualGids;
    std::vector<Cell> cells;
    std::unordered_map<std::string, sf::Vector2i> points;
    std::vector<TilesetData> tilesets;
};
