#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

class TextureManager {
public:
    std::shared_ptr<sf::Texture> loadTexture(const std::string& relativePath);

private:
    std::vector<std::string> assetCandidates(const std::string& relativePath) const;

    std::unordered_map<std::string, std::shared_ptr<sf::Texture>> textureCache_;
};
