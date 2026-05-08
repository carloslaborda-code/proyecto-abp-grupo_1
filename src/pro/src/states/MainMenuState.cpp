#include "states/MainMenuState.h"

MainMenuState::MainMenuState(sf::RenderWindow& gameWindow)
    : State(gameWindow),
      mainMenu(gameWindow) {
}

StateId MainMenuState::run() {
    return mainMenu.run() ? StateId::InGame : StateId::Exit;
}
