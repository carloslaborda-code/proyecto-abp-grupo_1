#include "systems/TextureManager.h"

#include <filesystem>

namespace fs = std::filesystem;

std::shared_ptr<sf::Texture> TextureManager::loadTexture(const std::string& relativePath) {
    const auto found = textureCache_.find(relativePath);
    if (found != textureCache_.end()) {
        return found->second;
    }

    for (const auto& candidate : assetCandidates(relativePath)) {
        if (!fs::exists(candidate)) {
            continue;
        }

        auto texture = std::make_shared<sf::Texture>();
        if (!texture->loadFromFile(candidate)) {
            continue;
        }

        textureCache_[relativePath] = texture;
        return texture;
    }

    return {};
}

std::vector<std::string> TextureManager::assetCandidates(const std::string& relativePath) const {
    return {
        relativePath,
        "resources/" + relativePath,
        "../resources/" + relativePath,
        "src/pro/BinaryAssault/resources/" + relativePath
    };
}
