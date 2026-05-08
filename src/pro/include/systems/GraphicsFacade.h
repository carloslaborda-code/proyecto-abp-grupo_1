#pragma once

#include <SFML/Graphics.hpp>

class GraphicsFacade {
public:
    void initializeWindow(sf::RenderWindow& window) const;
    void configureWorldView(sf::RenderWindow& window,
                            sf::View& worldView,
                            float worldWidth,
                            float worldHeight,
                            float reservedHudPixels) const;
    void beginFrame(sf::RenderWindow& window, const sf::Color& clearColor) const;
    void endFrame(sf::RenderWindow& window) const;
};
