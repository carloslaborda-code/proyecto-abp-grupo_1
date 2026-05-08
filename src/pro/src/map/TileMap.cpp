#include "map/TileMap.h"

#include <algorithm>
#include <filesystem>

#include <tinyxml2.h>

namespace {

namespace fs = std::filesystem;

std::vector<std::string> candidatePathsForAsset(const std::string& rawPath) {
    const fs::path path(rawPath);

    return {
        rawPath,
        (fs::path("..") / rawPath).string(),
        (fs::path("../..") / rawPath).string(),
        (fs::path("../../..") / rawPath).string()
    };
}

bool parseLayerData(tinyxml2::XMLElement* layer, std::vector<int>& outTiles) {
    outTiles.clear();
    if (!layer) {
        return false;
    }

    tinyxml2::XMLElement* data = layer->FirstChildElement("data");
    if (!data) {
        return false;
    }

    for (tinyxml2::XMLElement* tile = data->FirstChildElement("tile"); tile;
         tile = tile->NextSiblingElement("tile")) {
        outTiles.push_back(tile->IntAttribute("gid", 0));
    }

    return !outTiles.empty();
}

void parsePoints(tinyxml2::XMLElement* objectGroup,
                 int tileWidth,
                 int tileHeight,
                 std::unordered_map<std::string, sf::Vector2i>& outPoints) {
    outPoints.clear();
    if (!objectGroup) {
        return;
    }

    for (tinyxml2::XMLElement* object = objectGroup->FirstChildElement("object"); object;
         object = object->NextSiblingElement("object")) {
        const char* name = object->Attribute("name");
        if (!name) {
            continue;
        }

        const float xPixels = object->FloatAttribute("x", 0.f);
        const float yPixels = object->FloatAttribute("y", 0.f);
        const int tileX = tileWidth > 0 ? static_cast<int>(xPixels / tileWidth) : 0;
        const int tileY = tileHeight > 0 ? static_cast<int>(yPixels / tileHeight) : 0;
        outPoints[name] = {tileX, tileY};
    }
}

void parseInlineTilesets(tinyxml2::XMLElement* mapElement,
                         const fs::path& tmxPath,
                         std::vector<TilesetData>& outTilesets) {
    outTilesets.clear();
    if (!mapElement) {
        return;
    }

    for (tinyxml2::XMLElement* tilesetElement = mapElement->FirstChildElement("tileset"); tilesetElement;
         tilesetElement = tilesetElement->NextSiblingElement("tileset")) {
        if (tilesetElement->Attribute("source")) {
            continue;
        }

        TilesetData tileset;
        tileset.firstGid = tilesetElement->IntAttribute("firstgid", 0);
        tileset.tileWidth = tilesetElement->IntAttribute("tilewidth", 0);
        tileset.tileHeight = tilesetElement->IntAttribute("tileheight", 0);
        tileset.tileCount = tilesetElement->IntAttribute("tilecount", 0);
        tileset.columns = tilesetElement->IntAttribute("columns", 0);

        if (const char* name = tilesetElement->Attribute("name")) {
            tileset.name = name;
        }

        if (tinyxml2::XMLElement* image = tilesetElement->FirstChildElement("image")) {
            if (const char* source = image->Attribute("source")) {
                tileset.imageSource = source;
                tileset.resolvedImagePath =
                    (tmxPath.parent_path() / fs::path(tileset.imageSource)).lexically_normal().string();
            }
        }

        if (tileset.firstGid > 0) {
            outTilesets.push_back(tileset);
        }
    }

    std::sort(outTilesets.begin(), outTilesets.end(), [](const TilesetData& a, const TilesetData& b) {
        return a.firstGid < b.firstGid;
    });
}

}  // namespace

bool TileMap::loadFromTMX(const std::string& tmxPath) {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(tmxPath.c_str()) != tinyxml2::XML_SUCCESS) {
        return false;
    }

    tinyxml2::XMLElement* mapElement = document.FirstChildElement("map");
    if (!mapElement) {
        return false;
    }

    width = mapElement->IntAttribute("width", 0);
    height = mapElement->IntAttribute("height", 0);
    tileWidth = mapElement->IntAttribute("tilewidth", 0);
    tileHeight = mapElement->IntAttribute("tileheight", 0);
    if (width <= 0 || height <= 0 || tileWidth <= 0 || tileHeight <= 0) {
        return false;
    }

    tinyxml2::XMLElement* logicLayer = nullptr;
    tinyxml2::XMLElement* visualLayer = nullptr;
    tinyxml2::XMLElement* pointsLayer = nullptr;

    for (tinyxml2::XMLElement* child = mapElement->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* name = child->Attribute("name");
        const std::string layerName = name ? name : "";
        const std::string elementName = child->Name();

        if (elementName == "layer" && layerName == "Logic") {
            logicLayer = child;
        } else if (elementName == "layer" && layerName == "Visual") {
            visualLayer = child;
        } else if (elementName == "objectgroup" && layerName == "Points") {
            pointsLayer = child;
        }
    }

    if (!parseLayerData(logicLayer, logicGids) || !parseLayerData(visualLayer, visualGids)) {
        return false;
    }

    if (static_cast<int>(logicGids.size()) != width * height ||
        static_cast<int>(visualGids.size()) != width * height) {
        return false;
    }

    parsePoints(pointsLayer, tileWidth, tileHeight, points);
    parseInlineTilesets(mapElement, fs::path(tmxPath), tilesets);

    cells.clear();
    cells.reserve(logicGids.size());
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int index = y * width + x;
            cells.push_back({x, y, logicGids[index], visualGids[index]});
        }
    }

    loadTextures();
    return true;
}

void TileMap::draw(sf::RenderWindow& window) const {
    bool drewAnyTexturedTile = false;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int gid = visualGids[y * width + x];
            if (gid == 0) {
                continue;
            }

            const TilesetData* tileset = findTilesetForGid(gid);
            if (!tileset || !tileset->loaded || tileset->columns <= 0 || tileset->tileWidth <= 0 ||
                tileset->tileHeight <= 0) {
                continue;
            }

            const int localId = gid - tileset->firstGid;
            if (localId < 0) {
                continue;
            }

            const int textureX = localId % tileset->columns;
            const int textureY = localId / tileset->columns;

            sf::Sprite sprite(tileset->texture);
            sprite.setTextureRect(
                sf::IntRect(textureX * tileset->tileWidth,
                            textureY * tileset->tileHeight,
                            tileset->tileWidth,
                            tileset->tileHeight));
            sprite.setPosition(static_cast<float>(x * tileWidth), static_cast<float>(y * tileHeight));
            window.draw(sprite);
            drewAnyTexturedTile = true;
        }
    }

    if (!drewAnyTexturedTile) {
        drawFallback(window);
    }
}

int TileMap::getWidth() const {
    return width;
}

int TileMap::getHeight() const {
    return height;
}

int TileMap::getTileWidth() const {
    return tileWidth;
}

int TileMap::getTileHeight() const {
    return tileHeight;
}

const std::vector<int>& TileMap::getLogicGids() const {
    return logicGids;
}

const std::vector<int>& TileMap::getVisualGids() const {
    return visualGids;
}

const std::vector<Cell>& TileMap::getCells() const {
    return cells;
}

const std::unordered_map<std::string, sf::Vector2i>& TileMap::getPoints() const {
    return points;
}

bool TileMap::loadTextures() {
    for (auto& tileset : tilesets) {
        if (tileset.imageSource.empty()) {
            continue;
        }

        const std::vector<std::string> candidates = candidatePathsForAsset(tileset.resolvedImagePath);
        for (const auto& candidate : candidates) {
            if (candidate.empty()) {
                continue;
            }

            if (!fs::exists(candidate)) {
                continue;
            }

            if (!tileset.texture.loadFromFile(candidate)) {
                continue;
            }

            tileset.resolvedImagePath = candidate;
            tileset.loaded = true;
            break;
        }
    }

    return true;
}

const TilesetData* TileMap::findTilesetForGid(int gid) const {
    if (gid <= 0) {
        return nullptr;
    }

    const TilesetData* result = nullptr;
    for (const auto& tileset : tilesets) {
        if (tileset.firstGid <= gid) {
            result = &tileset;
        } else {
            break;
        }
    }

    return result;
}

void TileMap::drawFallback(sf::RenderWindow& window) const {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int gid = logicGids[y * width + x];
            sf::RectangleShape tile(sf::Vector2f(static_cast<float>(tileWidth), static_cast<float>(tileHeight)));
            tile.setPosition(static_cast<float>(x * tileWidth), static_cast<float>(y * tileHeight));

            if (gid == 14 || gid == 8) {
                tile.setFillColor(sf::Color(80, 95, 140));
            } else if (gid == 10 || gid == 13 || gid == 7) {
                tile.setFillColor(sf::Color(32, 32, 40));
            } else {
                tile.setFillColor(sf::Color(55, 55, 60));
            }

            tile.setOutlineThickness(-1.f);
            tile.setOutlineColor(sf::Color(20, 20, 20));
            window.draw(tile);
        }
    }
}
