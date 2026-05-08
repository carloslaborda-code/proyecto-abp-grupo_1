#include "systems/MatchFlowController.h"

#include <string>

#include <SFML/Graphics/Color.hpp>

#include "ui/MenuPausa.h"

void MatchFlowController::clearBanner(MatchFlowState& state) {
    state.bannerTitle.clear();
    state.bannerSubtitle.clear();
}

void MatchFlowController::prepareStage(MatchFlowState& state, bool awaitingStoryAcknowledgement) {
    state.paused = false;
    state.intermission = false;
    state.pendingStoryVictory = false;
    state.pendingStoryDefeat = false;
    state.awaitingIntermissionConfirmation = false;
    state.showingStoryIntermissionReport = false;
    state.awaitingStoryAcknowledgement = awaitingStoryAcknowledgement;
    clearBanner(state);
}

void MatchFlowController::resetRun(MatchFlowState& state) {
    state.gameOver = false;
    state.victory = false;
    prepareStage(state, false);
}

void MatchFlowController::startStoryIntermission(MatchFlowState& state,
                                                 int completedStoryStageIndex,
                                                 bool isFinalStoryStage) {
    state.intermission = true;
    state.pendingStoryVictory = isFinalStoryStage;
    state.pendingStoryDefeat = false;
    state.awaitingIntermissionConfirmation = true;
    state.showingStoryIntermissionReport = false;
    state.bannerTitle = "CAPITULO " + std::to_string(completedStoryStageIndex + 1) + " ASEGURADO";
    state.bannerSubtitle = isFinalStoryStage
                               ? "Haz clic para revisar el informe final antes del cierre de la operacion."
                               : "Haz clic para recibir el informe tactico de escalada.";
}

void MatchFlowController::startStoryDefeatIntermission(MatchFlowState& state, int failedStoryStageIndex) {
    state.intermission = true;
    state.pendingStoryVictory = false;
    state.pendingStoryDefeat = true;
    state.awaitingIntermissionConfirmation = true;
    state.showingStoryIntermissionReport = false;
    state.bannerTitle = "CAPITULO " + std::to_string(failedStoryStageIndex + 1) + " COMPROMETIDO";
    state.bannerSubtitle = "Haz clic para revisar el informe de colapso antes del cierre del operativo.";
}

void MatchFlowController::finalizeVictory(MatchFlowState& state,
                                          GameMode mode,
                                          int coreStability,
                                          int coreStabilityStart,
                                          int defensesBuilt,
                                          int coreHits,
                                          int infiniteTier,
                                          int score) {
    state.victory = true;
    state.paused = false;
    state.intermission = false;
    state.pendingStoryVictory = false;
    state.pendingStoryDefeat = false;
    state.awaitingIntermissionConfirmation = false;
    state.showingStoryIntermissionReport = false;
    state.awaitingStoryAcknowledgement = false;

    if (mode == GameMode::Story) {
        if (coreStability == coreStabilityStart && defensesBuilt <= 12 && coreHits == 0) {
            state.bannerTitle = "FINAL IMPECABLE";
            state.bannerSubtitle = "El nucleo se mantuvo estable y el protocolo nunca perdio el control.";
        } else if (coreStability >= 2) {
            state.bannerTitle = "FINAL ESTABLE";
            state.bannerSubtitle = "La amenaza fue contenida y el sistema vuelve a quedar bajo supervision segura.";
        } else {
            state.bannerTitle = "FINAL DE CONTINGENCIA";
            state.bannerSubtitle = "La red resistio a duras penas, pero el equipo tecnico logro cerrar la crisis.";
        }
        return;
    }

    if (mode == GameMode::Challenge) {
        state.bannerTitle = "CERTIFICACION DE HIERRO";
        state.bannerSubtitle = "Superaste el modo Desafio sin poder reparar el nucleo durante la simulacion.";
        return;
    }

    state.bannerTitle = "REGISTRO INFINITO";
    state.bannerSubtitle = "Sectores resistidos: " + std::to_string(infiniteTier + 1) +
                           "  |  Puntuacion final: " + std::to_string(score);
}

void MatchFlowController::finalizeDefeat(MatchFlowState& state, GameMode mode, int infiniteTier) {
    state.gameOver = true;
    state.paused = false;
    state.intermission = false;
    state.pendingStoryVictory = false;
    state.pendingStoryDefeat = false;
    state.awaitingIntermissionConfirmation = false;
    state.showingStoryIntermissionReport = false;
    state.awaitingStoryAcknowledgement = false;
    state.bannerTitle = "NUCLEO COMPROMETIDO";

    if (mode == GameMode::Infinite) {
        state.bannerSubtitle = "Resististe hasta el sector " + std::to_string(infiniteTier + 1) +
                               ". La simulacion ha terminado.";
        return;
    }

    state.bannerSubtitle = "La contencion ha fallado.";
}

void MatchFlowController::configurePauseOverlay(const MatchFlowState& state, MenuPausa& menuPausa) {
    if (state.paused) {
        menuPausa.configure("JUEGO EN PAUSA",
                            "",
                            {{
                                {"Continuar", sf::Color(125, 235, 150, 210), true},
                                {"Reiniciar simulacion", sf::Color(120, 220, 255, 210), true},
                                {"Volver al menu", sf::Color(255, 196, 87, 210), true},
                            }});
        return;
    }

    if (state.intermission) {
        menuPausa.configure("TRANSICION DE CAPITULO",
                            state.pendingStoryVictory
                                ? "Lee el informe final de etapa y cierra la operacion cuando quieras."
                                : (state.pendingStoryDefeat
                                       ? "Lee el informe de colapso y abre el cierre final cuando quieras."
                                       : "Lee el informe tactico y continua cuando quieras."),
                            {{
                                {state.pendingStoryVictory ? "Ver desenlace"
                                                           : (state.pendingStoryDefeat ? "Ver cierre" : "Siguiente etapa"),
                                 sf::Color(120, 220, 255, 210),
                                 true},
                                {"Volver al menu", sf::Color(255, 196, 87, 210), true},
                                {"", sf::Color::White, false},
                            }},
                            {500.f, 248.f});
        return;
    }

    if (state.gameOver || state.victory) {
        menuPausa.configure(state.bannerTitle,
                            state.bannerSubtitle,
                            {{
                                {"Reiniciar simulacion", sf::Color(120, 220, 255, 210), true},
                                {"Volver al menu", sf::Color(255, 196, 87, 210), true},
                                {"", sf::Color::White, false},
                            }},
                            {500.f, 248.f});
    }
}

bool MatchFlowController::canAdvanceSimulation(const MatchFlowState& state) {
    return !state.paused && !state.gameOver && !state.victory && !state.intermission;
}
