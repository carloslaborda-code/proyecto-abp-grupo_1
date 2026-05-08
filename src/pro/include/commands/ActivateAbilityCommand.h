#pragma once

#include <functional>

#include "commands/Command.h"

class ActivateAbilityCommand : public Command {
public:
    using ExecuteFn = std::function<bool()>;

    explicit ActivateAbilityCommand(ExecuteFn executeFn);

    bool execute() override;

private:
    ExecuteFn executeFn_;
};
