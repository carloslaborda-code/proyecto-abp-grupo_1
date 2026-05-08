#pragma once

#include "states/State.h"
#include "ui/MainMenu.h"

class MainMenuState : public State {
public:
    explicit MainMenuState(sf::RenderWindow& gameWindow);

    StateId run() override;

private:
    MainMenu mainMenu;
};
