#pragma once

#include <functional>

#include "commands/Command.h"
#include "entities/Defense.h"
#include "systems/EconomySystem.h"

class PlaceDefenseCommand : public Command {
public:
    using ConfigureDefenseFn = std::function<void(Defense&, DefenseType)>;

    PlaceDefenseCommand(Defense& defense,
                        DefenseType type,
                        EconomySystem& economy,
                        int cost,
                        ConfigureDefenseFn configureDefense);

    bool execute() override;

private:
    Defense& defense_;
    DefenseType type_;
    EconomySystem& economy_;
    int cost_;
    ConfigureDefenseFn configureDefense_;
};
