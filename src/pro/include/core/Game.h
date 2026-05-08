#pragma once
#include <SFML/Graphics.hpp>

#include "states/State.h"
#include "systems/GraphicsFacade.h"

class Game {
private:
    sf::RenderWindow window;
    GraphicsFacade graphicsFacade;

public:
    Game();
    void run();
};
