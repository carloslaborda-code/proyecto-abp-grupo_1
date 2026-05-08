#pragma once

#include <SFML/Graphics.hpp>

enum class StateId {
    MainMenu,
    InGame,
    Exit
};

class State {
public:
    explicit State(sf::RenderWindow& gameWindow);
    virtual ~State() = default;

    virtual StateId run() = 0;

protected:
    sf::RenderWindow& window;
};
