#pragma once

class EconomySystem {
public:
    explicit EconomySystem(int initialCredits = 110);

    int getCredits() const;
    bool canAfford(int amount) const;
    bool spend(int amount);
    void earn(int amount);

private:
    int credits;
};
