#include "commands/ActivateAbilityCommand.h"

ActivateAbilityCommand::ActivateAbilityCommand(ExecuteFn executeFn)
    : executeFn_(std::move(executeFn)) {
}

bool ActivateAbilityCommand::execute() {
    if (!executeFn_) {
        return false;
    }

    return executeFn_();
}
