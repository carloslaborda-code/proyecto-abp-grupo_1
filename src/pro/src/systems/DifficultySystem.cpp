#include "systems/DifficultySystem.h"

#include <algorithm>

namespace {

float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

}  // namespace

DifficultySystem::DifficultySystem()
    : currentMode(GameMode::Story),
      currentStage(0),
      initialCredits(120),
      coreRepairEnabled(true),
      reliefFactorFromPreviousStage(1.f),
      baseEnemySpeed(1.f),
      baseEnemyHealth(1.f),
      baseSpawnInterval(1.f),
      resourcesMultiplierValue(1.f),
      coreHitsInStage(0),
      defeatedEnemiesInStage(0),
      distanceAccumulator(0.f) {
}

void DifficultySystem::startRun(GameMode mode, int laneCount) {
    currentMode = mode;
    currentStage = 0;
    reliefFactorFromPreviousStage = 1.f;
    coreHitsInStage = 0;
    defeatedEnemiesInStage = 0;
    distanceAccumulator = 0.f;
    resizeLaneTracking(laneCount);

    switch (currentMode) {
        case GameMode::Story:
            initialCredits = 108;
            break;
        case GameMode::Challenge:
            initialCredits = 108;
            break;
        case GameMode::Infinite:
            initialCredits = 120;
            break;
    }
}

void DifficultySystem::startStage(int stageIndex, int laneCount) {
    if (defeatedEnemiesInStage > 0) {
        const float averageDistance = distanceAccumulator / static_cast<float>(defeatedEnemiesInStage);
        if (averageDistance <= 0.33f) {
            reliefFactorFromPreviousStage = 0.92f;
        } else if (averageDistance >= 0.72f) {
            reliefFactorFromPreviousStage = 1.05f;
        } else {
            reliefFactorFromPreviousStage = 1.f;
        }
    }

    currentStage = stageIndex;
    coreHitsInStage = 0;
    defeatedEnemiesInStage = 0;
    distanceAccumulator = 0.f;
    resizeLaneTracking(laneCount);

    const float stageFactor = static_cast<float>(stageIndex);
    switch (currentMode) {
        case GameMode::Story:
            coreRepairEnabled = true;
            baseEnemySpeed = (0.94f + 0.09f * stageFactor) * reliefFactorFromPreviousStage;
            baseEnemyHealth = (0.90f + 0.13f * stageFactor) * reliefFactorFromPreviousStage;
            baseSpawnInterval = clampf((1.22f - 0.08f * stageFactor) / reliefFactorFromPreviousStage, 0.78f, 1.30f);
            resourcesMultiplierValue = clampf(1.00f - 0.04f * stageFactor, 0.90f, 1.00f);
            break;
        case GameMode::Challenge:
            coreRepairEnabled = false;
            baseEnemySpeed = (1.04f + 0.06f * stageFactor) * reliefFactorFromPreviousStage;
            baseEnemyHealth = (1.10f + 0.10f * stageFactor) * reliefFactorFromPreviousStage;
            baseSpawnInterval =
                clampf((1.02f - 0.03f * stageFactor) / reliefFactorFromPreviousStage, 0.70f, 1.08f);
            resourcesMultiplierValue = 0.98f;
            break;
        case GameMode::Infinite:
            coreRepairEnabled = true;
            baseEnemySpeed = (0.88f + 0.06f * stageFactor) * reliefFactorFromPreviousStage;
            baseEnemyHealth = (0.90f + 0.09f * stageFactor) * reliefFactorFromPreviousStage;
            baseSpawnInterval =
                clampf((1.16f - 0.035f * stageFactor) / reliefFactorFromPreviousStage, 0.68f, 1.20f);
            resourcesMultiplierValue = clampf(1.03f - 0.01f * stageFactor, 0.90f, 1.03f);
            break;
    }
}

void DifficultySystem::update(float dt, const std::vector<Defense>& defenses) {
    float firstThreshold = 5.f;
    float secondThreshold = 8.f;
    float thirdThreshold = 11.f;
    float firstMultiplier = 1.08f;
    float secondMultiplier = 1.16f;
    float thirdMultiplier = 1.24f;

    if (currentMode == GameMode::Story) {
        if (currentStage <= 0) {
            firstThreshold = 7.f;
            secondThreshold = 11.f;
            thirdThreshold = 15.f;
            firstMultiplier = 1.04f;
            secondMultiplier = 1.08f;
            thirdMultiplier = 1.12f;
        } else if (currentStage == 1) {
            firstThreshold = 6.f;
            secondThreshold = 9.f;
            thirdThreshold = 13.f;
            firstMultiplier = 1.06f;
            secondMultiplier = 1.11f;
            thirdMultiplier = 1.17f;
        } else {
            firstMultiplier = 1.08f;
            secondMultiplier = 1.14f;
            thirdMultiplier = 1.20f;
        }
    } else if (currentMode == GameMode::Challenge) {
        firstThreshold = 7.0f;
        secondThreshold = 10.0f;
        thirdThreshold = 13.0f;
        firstMultiplier = 1.08f;
        secondMultiplier = 1.16f;
        thirdMultiplier = 1.26f;
    } else if (currentMode == GameMode::Infinite) {
        firstThreshold = std::max(5.0f, 6.5f - 0.08f * static_cast<float>(currentStage));
        secondThreshold = std::max(7.5f, 10.0f - 0.10f * static_cast<float>(currentStage));
        thirdThreshold = std::max(10.0f, 13.5f - 0.12f * static_cast<float>(currentStage));
        firstMultiplier = std::min(1.14f, 1.04f + 0.008f * static_cast<float>(currentStage));
        secondMultiplier = std::min(1.24f, 1.10f + 0.010f * static_cast<float>(currentStage));
        thirdMultiplier = std::min(1.36f, 1.18f + 0.012f * static_cast<float>(currentStage));
    }

    for (std::size_t laneId = 0; laneId < laneIdleTimers.size(); ++laneId) {
        bool laneHasDefense = false;
        for (const auto& defense : defenses) {
            if (defense.laneId == static_cast<int>(laneId) && defense.placed && defense.hp > 0.f) {
                laneHasDefense = true;
                break;
            }
        }

        if (laneHasDefense) {
            laneIdleTimers[laneId] = std::max(0.f, laneIdleTimers[laneId] - dt * 1.2f);
        } else {
            laneIdleTimers[laneId] += dt;
        }

        const float idleTime = laneIdleTimers[laneId];
        if (idleTime >= thirdThreshold) {
            lanePressureMultipliers[laneId] = thirdMultiplier;
        } else if (idleTime >= secondThreshold) {
            lanePressureMultipliers[laneId] = secondMultiplier;
        } else if (idleTime >= firstThreshold) {
            lanePressureMultipliers[laneId] = firstMultiplier;
        } else {
            lanePressureMultipliers[laneId] = 1.f;
        }
    }
}

void DifficultySystem::notifyCoreHit() {
    ++coreHitsInStage;
}

void DifficultySystem::notifyEnemyDefeated(int laneId, float normalizedDistanceToCore) {
    ++defeatedEnemiesInStage;
    distanceAccumulator += clampf(normalizedDistanceToCore, 0.f, 1.f);

    if (laneId >= 0 && laneId < static_cast<int>(laneIdleTimers.size())) {
        laneIdleTimers[laneId] = std::max(0.f, laneIdleTimers[laneId] - 1.2f);
    }
}

float DifficultySystem::enemySpeedMultiplier() const {
    float hitPenalty = 0.04f;
    float maxPenalty = 0.20f;

    if (currentMode == GameMode::Story) {
        if (currentStage <= 0) {
            hitPenalty = 0.02f;
            maxPenalty = 0.08f;
        } else if (currentStage == 1) {
            hitPenalty = 0.025f;
            maxPenalty = 0.10f;
        } else {
            hitPenalty = 0.03f;
            maxPenalty = 0.12f;
        }
    } else if (currentMode == GameMode::Challenge) {
        hitPenalty = 0.03f;
        maxPenalty = 0.12f;
    } else if (currentMode == GameMode::Infinite) {
        hitPenalty = std::min(0.035f, 0.018f + 0.0015f * static_cast<float>(currentStage));
        maxPenalty = std::min(0.16f, 0.08f + 0.009f * static_cast<float>(currentStage));
    }

    return baseEnemySpeed + std::min(maxPenalty, hitPenalty * static_cast<float>(coreHitsInStage));
}

float DifficultySystem::enemyHealthMultiplier() const {
    return baseEnemyHealth;
}

float DifficultySystem::spawnIntervalMultiplier() const {
    float hitAcceleration = 0.03f;
    float minimumMultiplier = 0.45f;

    if (currentMode == GameMode::Story) {
        if (currentStage <= 0) {
            hitAcceleration = 0.015f;
            minimumMultiplier = 0.70f;
        } else if (currentStage == 1) {
            hitAcceleration = 0.02f;
            minimumMultiplier = 0.62f;
        } else {
            hitAcceleration = 0.025f;
            minimumMultiplier = 0.55f;
        }
    } else if (currentMode == GameMode::Challenge) {
        hitAcceleration = 0.025f;
        minimumMultiplier = 0.60f;
    } else if (currentMode == GameMode::Infinite) {
        hitAcceleration = std::min(0.026f, 0.012f + 0.0012f * static_cast<float>(currentStage));
        minimumMultiplier = std::max(0.58f, 0.78f - 0.010f * static_cast<float>(currentStage));
    }

    return std::max(minimumMultiplier, baseSpawnInterval - hitAcceleration * static_cast<float>(coreHitsInStage));
}

float DifficultySystem::resourceMultiplier() const {
    return resourcesMultiplierValue;
}

float DifficultySystem::laneSpeedMultiplier(int laneId) const {
    if (laneId < 0 || laneId >= static_cast<int>(lanePressureMultipliers.size())) {
        return 1.f;
    }

    return lanePressureMultipliers[laneId];
}

int DifficultySystem::passiveIncomeAmount() const {
    switch (currentMode) {
        case GameMode::Story:
            return currentStage >= 2 ? 3 : 2;
        case GameMode::Challenge:
            return 2;
        case GameMode::Infinite:
            return currentStage >= 6 ? 4 : (currentStage >= 2 ? 3 : 2);
    }

    return 1;
}

float DifficultySystem::passiveIncomeInterval() const {
    switch (currentMode) {
        case GameMode::Story:
            if (currentStage <= 0) {
                return 4.5f;
            }
            if (currentStage == 1) {
                return 4.0f;
            }
            return 3.5f;
        case GameMode::Challenge:
            return 6.0f;
        case GameMode::Infinite:
            return std::max(3.0f, 5.2f - 0.12f * static_cast<float>(currentStage));
    }

    return 5.f;
}

bool DifficultySystem::allowCoreRepair() const {
    return coreRepairEnabled;
}

int DifficultySystem::startingCredits() const {
    return initialCredits;
}

GameMode DifficultySystem::mode() const {
    return currentMode;
}

int DifficultySystem::stageIndex() const {
    return currentStage;
}

void DifficultySystem::resizeLaneTracking(int laneCount) {
    const int safeLaneCount = std::max(0, laneCount);
    laneIdleTimers.assign(static_cast<std::size_t>(safeLaneCount), 0.f);
    lanePressureMultipliers.assign(static_cast<std::size_t>(safeLaneCount), 1.f);
}
