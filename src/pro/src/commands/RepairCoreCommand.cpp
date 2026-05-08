#include "commands/RepairCoreCommand.h"

#include <algorithm>

RepairCoreCommand::RepairCoreCommand(Core& core,
                                     DifficultySystem& difficulty,
                                     EconomySystem& economy,
                                     int cost,
                                     int maxCoreStability,
                                     float& repairCooldown,
                                     float cooldownSeconds)
    : core_(core),
      difficulty_(difficulty),
      economy_(economy),
      cost_(cost),
      maxCoreStability_(maxCoreStability),
      repairCooldown_(repairCooldown),
      cooldownSeconds_(cooldownSeconds) {
}

bool RepairCoreCommand::execute() {
    if (!difficulty_.allowCoreRepair() || repairCooldown_ > 0.f) {
        return false;
    }

    if (core_.stability >= maxCoreStability_) {
        return false;
    }

    if (!economy_.spend(cost_)) {
        return false;
    }

    core_.stability = std::min(maxCoreStability_, core_.stability + 1);
    repairCooldown_ = cooldownSeconds_;
    return true;
}
