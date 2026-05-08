#pragma once

#include "commands/Command.h"
#include "entities/Core.h"
#include "systems/DifficultySystem.h"
#include "systems/EconomySystem.h"

class RepairCoreCommand : public Command {
public:
    RepairCoreCommand(Core& core,
                      DifficultySystem& difficulty,
                      EconomySystem& economy,
                      int cost,
                      int maxCoreStability,
                      float& repairCooldown,
                      float cooldownSeconds);

    bool execute() override;

private:
    Core& core_;
    DifficultySystem& difficulty_;
    EconomySystem& economy_;
    int cost_;
    int maxCoreStability_;
    float& repairCooldown_;
    float cooldownSeconds_;
};
