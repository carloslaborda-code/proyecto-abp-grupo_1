#pragma once

#include <optional>

#include <SFML/Graphics.hpp>

#include "entities/Defense.h"

enum class InGameInputActionType {
    None,
    AcknowledgeStory,
    Pause,
    RepairCore,
    UseLaneBoost,
    UseAbsorption,
    SelectDefenseCard,
    BuildDefense,
    SellDefense,
    SelectSlot
};

struct InGameInputContext {
    bool simulationActive = false;
    bool awaitingStoryAcknowledgement = false;
    bool storyPanelHit = false;
    bool pauseButtonHit = false;
    sf::Vector2u windowSize;
    sf::FloatRect worldViewport;
    sf::Vector2i pixel;
    std::optional<DefenseType> selectedCard;
    int clickedSlot = -1;
    bool clickedSlotOccupied = false;
};

struct InGameInputAction {
    InGameInputActionType type = InGameInputActionType::None;
    int index = -1;
    std::optional<DefenseType> defenseType;
};

class InGameInputHandler {
public:
    static InGameInputAction interpretLeftClick(const InGameInputContext& context);
    static InGameInputAction interpretRightClick(const InGameInputContext& context);
};
