#pragma once

#include <string>

#include "core/Config.h"

class MenuPausa;

struct MatchFlowState {
    bool gameOver = false;
    bool victory = false;
    bool paused = false;
    bool intermission = false;
    bool pendingStoryVictory = false;
    bool pendingStoryDefeat = false;
    bool awaitingIntermissionConfirmation = false;
    bool showingStoryIntermissionReport = false;
    bool awaitingStoryAcknowledgement = false;
    std::string bannerTitle;
    std::string bannerSubtitle;
};

class MatchFlowController {
public:
    static void clearBanner(MatchFlowState& state);
    static void prepareStage(MatchFlowState& state, bool awaitingStoryAcknowledgement);
    static void resetRun(MatchFlowState& state);
    static void startStoryIntermission(MatchFlowState& state, int completedStoryStageIndex, bool isFinalStoryStage);
    static void startStoryDefeatIntermission(MatchFlowState& state, int failedStoryStageIndex);
    static void finalizeVictory(MatchFlowState& state,
                                GameMode mode,
                                int coreStability,
                                int coreStabilityStart,
                                int defensesBuilt,
                                int coreHits,
                                int infiniteTier,
                                int score);
    static void finalizeDefeat(MatchFlowState& state, GameMode mode, int infiniteTier);
    static void configurePauseOverlay(const MatchFlowState& state, MenuPausa& menuPausa);
    static bool canAdvanceSimulation(const MatchFlowState& state);
};
