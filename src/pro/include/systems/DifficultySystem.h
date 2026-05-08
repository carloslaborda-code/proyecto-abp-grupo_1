#pragma once

#include <vector>

#include "core/Config.h"
#include "entities/Defense.h"

class DifficultySystem {
public:
    DifficultySystem();

    void startRun(GameMode mode, int laneCount);
    void startStage(int stageIndex, int laneCount);
    void update(float dt, const std::vector<Defense>& defenses);
    void notifyCoreHit();
    void notifyEnemyDefeated(int laneId, float normalizedDistanceToCore);

    float enemySpeedMultiplier() const;
    float enemyHealthMultiplier() const;
    float spawnIntervalMultiplier() const;
    float resourceMultiplier() const;
    float laneSpeedMultiplier(int laneId) const;
    int passiveIncomeAmount() const;
    float passiveIncomeInterval() const;
    bool allowCoreRepair() const;
    int startingCredits() const;
    GameMode mode() const;
    int stageIndex() const;

private:
    void resizeLaneTracking(int laneCount);

    GameMode currentMode;
    int currentStage;
    int initialCredits;
    bool coreRepairEnabled;
    float reliefFactorFromPreviousStage;
    float baseEnemySpeed;
    float baseEnemyHealth;
    float baseSpawnInterval;
    float resourcesMultiplierValue;
    int coreHitsInStage;
    int defeatedEnemiesInStage;
    float distanceAccumulator;
    std::vector<float> laneIdleTimers;
    std::vector<float> lanePressureMultipliers;
};
