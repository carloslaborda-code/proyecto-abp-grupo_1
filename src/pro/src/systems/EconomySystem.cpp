#include "systems/EconomySystem.h"

EconomySystem::EconomySystem(int initialCredits)
    : credits(initialCredits) {
}

int EconomySystem::getCredits() const {
    return credits;
}

bool EconomySystem::canAfford(int amount) const {
    return credits >= amount;
}

bool EconomySystem::spend(int amount) {
    if (!canAfford(amount)) {
        return false;
    }

    credits -= amount;
    return true;
}

void EconomySystem::earn(int amount) {
    credits += amount;
}
