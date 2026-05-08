#include "commands/PlaceDefenseCommand.h"

PlaceDefenseCommand::PlaceDefenseCommand(Defense& defense,
                                         DefenseType type,
                                         EconomySystem& economy,
                                         int cost,
                                         ConfigureDefenseFn configureDefense)
    : defense_(defense),
      type_(type),
      economy_(economy),
      cost_(cost),
      configureDefense_(std::move(configureDefense)) {
}

bool PlaceDefenseCommand::execute() {
    if (defense_.placed) {
        return false;
    }

    if (!economy_.spend(cost_)) {
        return false;
    }

    defense_.placed = true;
    configureDefense_(defense_, type_);
    return true;
}
