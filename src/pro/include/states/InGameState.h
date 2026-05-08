#pragma once

#include <SFML/Graphics.hpp>

#include "states/State.h"

class InGameState : public State {
public:
    explicit InGameState(sf::RenderWindow& gameWindow);

    StateId run() override;
};
