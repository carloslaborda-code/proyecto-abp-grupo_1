#include "core/Game.h"

#include <memory>

#include "states/MainMenuState.h"
#include "states/InGameState.h"

Game::Game()
    : window() {
    graphicsFacade.initializeWindow(window);
}

void Game::run() {
    StateId currentStateId = StateId::MainMenu;

    while (window.isOpen() && currentStateId != StateId::Exit) {
        std::unique_ptr<State> currentState;
        if (currentStateId == StateId::MainMenu) {
            currentState = std::make_unique<MainMenuState>(window);
        } else {
            currentState = std::make_unique<InGameState>(window);
        }

        currentStateId = currentState->run();
    }
}
