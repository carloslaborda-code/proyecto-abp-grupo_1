#include "input/InGameInputHandler.h"

#include "ui/HUD.h"

InGameInputAction InGameInputHandler::interpretLeftClick(const InGameInputContext& context) {
    if (!context.simulationActive) {
        return {};
    }

    if (context.awaitingStoryAcknowledgement) {
        if (context.storyPanelHit) {
            return {InGameInputActionType::AcknowledgeStory, -1, std::nullopt};
        }
        return {};
    }

    if (context.pauseButtonHit) {
        return {InGameInputActionType::Pause, -1, std::nullopt};
    }

    const sf::Vector2f uiPoint(static_cast<float>(context.pixel.x), static_cast<float>(context.pixel.y));
    const HUDActionLayout actionLayout = HUD::actionLayout(context.windowSize, context.worldViewport);
    if (actionLayout.valid) {
        if (actionLayout.repairRect.contains(uiPoint)) {
            return {InGameInputActionType::RepairCore, -1, std::nullopt};
        }

        if (actionLayout.boostRect.contains(uiPoint)) {
            return {InGameInputActionType::UseLaneBoost, -1, std::nullopt};
        }

        for (std::size_t i = 0; i < actionLayout.absorptionRects.size(); ++i) {
            if (actionLayout.absorptionRects[i].contains(uiPoint)) {
                return {InGameInputActionType::UseAbsorption, static_cast<int>(i), std::nullopt};
            }
        }
    }

    if (context.selectedCard) {
        return {InGameInputActionType::SelectDefenseCard, -1, context.selectedCard};
    }

    if (context.clickedSlot >= 0) {
        return {InGameInputActionType::BuildDefense, context.clickedSlot, std::nullopt};
    }

    return {};
}

InGameInputAction InGameInputHandler::interpretRightClick(const InGameInputContext& context) {
    if (!context.simulationActive || context.clickedSlot < 0) {
        return {};
    }

    if (context.clickedSlotOccupied) {
        return {InGameInputActionType::SellDefense, context.clickedSlot, std::nullopt};
    }

    return {InGameInputActionType::SelectSlot, context.clickedSlot, std::nullopt};
}
