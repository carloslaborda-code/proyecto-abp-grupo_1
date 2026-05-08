#include "systems/GraphicsFacade.h"

#include <algorithm>

#include "core/Config.h"

void GraphicsFacade::initializeWindow(sf::RenderWindow& window) const {
    const Config& config = Config::getInstance();
    window.create(sf::VideoMode(config.windowWidth(), config.windowHeight()), config.windowTitle());
    window.setFramerateLimit(config.targetFps());
}

void GraphicsFacade::configureWorldView(sf::RenderWindow& window,
                                        sf::View& worldView,
                                        float worldWidth,
                                        float worldHeight,
                                        float reservedHudPixels) const {
    const sf::Vector2u size = window.getSize();
    if (size.x == 0 || size.y == 0 || worldHeight <= 0.f) {
        return;
    }

    const float usableHeight = std::max(1.f, static_cast<float>(size.y) - reservedHudPixels);
    const float windowAspect = static_cast<float>(size.x) / usableHeight;
    const float worldAspect = worldWidth / worldHeight;

    sf::FloatRect viewport(0.f, 0.f, 1.f, usableHeight / static_cast<float>(size.y));
    if (windowAspect > worldAspect) {
        viewport.width = worldAspect / windowAspect;
        viewport.left = (1.f - viewport.width) * 0.5f;
    } else if (windowAspect < worldAspect) {
        viewport.height = (static_cast<float>(size.x) / worldAspect) / static_cast<float>(size.y);
        viewport.top = (usableHeight - viewport.height * static_cast<float>(size.y)) * 0.5f /
                       static_cast<float>(size.y);
    }

    worldView.setViewport(viewport);
    window.setView(worldView);
}

void GraphicsFacade::beginFrame(sf::RenderWindow& window, const sf::Color& clearColor) const {
    window.clear(clearColor);
}

void GraphicsFacade::endFrame(sf::RenderWindow& window) const {
    window.display();
}
