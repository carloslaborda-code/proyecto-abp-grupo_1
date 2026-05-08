#pragma once

#include <array>

#include "entities/Enemy.h"

class AbsorptionSystem {
public:
    AbsorptionSystem();

    void reset();
    void notifyEnemyDefeated(EnemyType type);
    void grantCharge(EnemyType type, int count = 1);
    void grantAllCharges(int count = 1);

    bool hasCharge(EnemyType type) const;
    bool consume(EnemyType type);
    int charges(EnemyType type) const;
    int progress(EnemyType type) const;
    int threshold(EnemyType type) const;
    int remainingToUnlock(EnemyType type) const;

private:
    struct Track {
        int progress = 0;
        int charges = 0;
        int threshold = 0;
    };

    static std::size_t indexFor(EnemyType type);

    std::array<Track, 4> tracks;
};
