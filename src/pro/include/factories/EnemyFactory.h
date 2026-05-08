#pragma once

#include "entities/Enemy.h"
#include "map/Level.h"

class EnemyFactory {
public:
    static Enemy createEnemy(EnemyType type,
                             int laneId,
                             const Level& level,
                             float standardSpeed,
                             float fastSpeed,
                             float heavySpeed,
                             float adaptiveSpeed);
};
