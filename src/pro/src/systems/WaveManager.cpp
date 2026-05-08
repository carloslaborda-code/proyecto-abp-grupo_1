#include "systems/WaveManager.h"

#include <algorithm>
#include <cmath>

WaveManager::WaveManager()
    : rng(std::random_device{}()),
      currentMode(GameMode::Story),
      currentStageIndex(0),
      currentWaveIndex(0),
      spawnedInWave(0),
      lastSpawnLaneId(-1),
      spawnTimer(0.f),
      preparationTimer(0.f),
      initialPreparationTime(10.f),
      betweenWavePreparationTime(5.f),
      initialPreparationCreditsValue(0),
      betweenWavePreparationCreditsValue(0),
      phase(WavePhase::Preparation) {
    configure(GameMode::Story, 0);
}

void WaveManager::configure(GameMode mode, int stageIndex) {
    currentMode = mode;
    currentStageIndex = std::max(0, stageIndex);
    waves.clear();

    switch (currentMode) {
        case GameMode::Story:
            setStoryStageWaves(currentStageIndex);
            break;
        case GameMode::Challenge:
            setChallengeWaves(currentStageIndex);
            break;
        case GameMode::Infinite:
            setInfiniteWaves(currentStageIndex);
            break;
    }

    configurePreparationValues(currentMode, currentStageIndex);
    currentWaveIndex = 0;
    spawnedInWave = 0;
    lastSpawnLaneId = -1;
    spawnTimer = 0.f;
    preparationTimer = initialPreparationTime;
    phase = WavePhase::Preparation;
}

WaveUpdateResult WaveManager::update(float dt,
                                     int laneCount,
                                     int aliveEnemies,
                                     float spawnIntervalMultiplier,
                                     int defeatedEnemies) {
    WaveUpdateResult result;
    if (laneCount <= 0 || currentWaveIndex >= static_cast<int>(waves.size())) {
        return result;
    }

    if (phase == WavePhase::Preparation) {
        preparationTimer = std::max(0.f, preparationTimer - dt);
        if (preparationTimer <= 0.f) {
            phase = WavePhase::Spawning;
            spawnTimer = 0.f;
        }
        return result;
    }

    const WaveDefinition& wave = waves[currentWaveIndex];

    if (phase == WavePhase::Spawning) {
        if (spawnedInWave < wave.enemyCount) {
            const float effectiveInterval = std::max(0.16f, wave.spawnInterval * spawnIntervalMultiplier);
            const int maxAliveEnemies =
                wave.maxAliveEnemies > 0 ? std::min(wave.maxAliveEnemies, wave.enemyCount) : wave.enemyCount;
            if (aliveEnemies >= maxAliveEnemies) {
                spawnTimer = std::min(spawnTimer, effectiveInterval);
                return result;
            }

            spawnTimer += dt;
            if (spawnTimer >= effectiveInterval) {
                spawnTimer -= effectiveInterval;
                ++spawnedInWave;

                if (spawnedInWave >= wave.enemyCount) {
                    phase = WavePhase::Cleanup;
                }

                result.spawn = SpawnRequest{rollEnemyType(wave, defeatedEnemies), chooseSpawnLane(laneCount)};
            }
            return result;
        }

        phase = WavePhase::Cleanup;
    }

    if (phase == WavePhase::Cleanup && aliveEnemies == 0) {
        if (currentWaveIndex + 1 >= static_cast<int>(waves.size())) {
            phase = WavePhase::Completed;
            return result;
        }

        ++currentWaveIndex;
        spawnedInWave = 0;
        lastSpawnLaneId = -1;
        spawnTimer = 0.f;
        preparationTimer = betweenWavePreparationTime;
        phase = WavePhase::Preparation;
        result.preparationCredits = betweenWavePreparationCreditsValue;
        result.enteredPreparation = true;
    }

    return result;
}

bool WaveManager::hasStageVictory(int aliveEnemies) const {
    return phase == WavePhase::Completed && aliveEnemies == 0;
}

int WaveManager::currentWaveNumber() const {
    if (waves.empty()) {
        return 0;
    }

    return std::min(currentWaveIndex + 1, static_cast<int>(waves.size()));
}

int WaveManager::totalWaveCount() const {
    return static_cast<int>(waves.size());
}

int WaveManager::totalEnemiesInStage() const {
    int total = 0;
    for (const auto& wave : waves) {
        total += wave.enemyCount;
    }
    return total;
}

bool WaveManager::isPreparing() const {
    return phase == WavePhase::Preparation;
}

float WaveManager::preparationTimeRemaining() const {
    return preparationTimer;
}

int WaveManager::preparationSecondsRemaining() const {
    const float safeRemaining = std::max(0.f, preparationTimer);
    if (safeRemaining <= 0.f) {
        return 0;
    }
    return static_cast<int>(std::ceil(safeRemaining - 0.001f));
}

int WaveManager::initialPreparationCredits() const {
    return initialPreparationCreditsValue;
}

int WaveManager::chooseSpawnLane(int laneCount) {
    if (laneCount <= 1) {
        lastSpawnLaneId = 0;
        return 0;
    }

    if (currentMode == GameMode::Story && currentStageIndex == 0) {
        const int laneId = (currentWaveIndex + std::max(0, spawnedInWave - 1)) % laneCount;
        lastSpawnLaneId = laneId;
        return laneId;
    }

    std::uniform_int_distribution<int> laneDist(0, laneCount - 1);
    int laneId = laneDist(rng);
    if (laneId == lastSpawnLaneId) {
        laneId = (laneId + 1 + (spawnedInWave % (laneCount - 1))) % laneCount;
    }

    lastSpawnLaneId = laneId;
    return laneId;
}

EnemyType WaveManager::rollEnemyType(const WaveDefinition& wave, int defeatedEnemies) {
    const WaveDefinition weightedWave =
        currentMode == GameMode::Infinite ? infiniteWeightsForDefeatedEnemies(defeatedEnemies) : wave;
    std::uniform_int_distribution<int> dist(1, 100);
    const int roll = dist(rng);
    if (roll <= weightedWave.heavyWeight) {
        return EnemyType::Heavy;
    }
    if (roll <= weightedWave.heavyWeight + weightedWave.fastWeight) {
        return EnemyType::Fast;
    }
    if (roll <= weightedWave.heavyWeight + weightedWave.fastWeight + weightedWave.adaptiveWeight) {
        return EnemyType::Adaptive;
    }
    return EnemyType::Standard;
}

WaveManager::WaveDefinition WaveManager::infiniteWeightsForDefeatedEnemies(int defeatedEnemies) const {
    const int kills = std::max(0, defeatedEnemies);
    if (kills >= 120) {
        return {0, 0.f, 0, 24, 32, 24};
    }
    if (kills >= 80) {
        return {0, 0.f, 0, 22, 28, 18};
    }
    if (kills >= 50) {
        return {0, 0.f, 0, 18, 22, 12};
    }
    if (kills >= 24) {
        return {0, 0.f, 0, 14, 16, 6};
    }

    return {0, 0.f, 0, 8, 8, 0};
}

void WaveManager::setStoryStageWaves(int stageIndex) {
    // WaveDefinition: enemyCount, spawnInterval, maxAlive, fast%, heavy%, adaptive%.
    // Standard uses the remaining probability up to 100.
    switch (stageIndex) {
        case 0:
            waves = {
                {5, 2.95f, 3, 0, 18, 0},
                {7, 2.55f, 4, 8, 20, 0},
                {9, 2.20f, 5, 12, 24, 0},
            };
            break;
        case 1:
            waves = {
                {10, 2.05f, 5, 12, 18, 8},
                {13, 1.78f, 6, 16, 22, 12},
                {15, 1.56f, 7, 18, 26, 16},
            };
            break;
        default:
            waves = {
                {16, 1.72f, 7, 16, 24, 14},
                {19, 1.46f, 8, 20, 28, 18},
                {22, 1.24f, 9, 24, 32, 20},
            };
            break;
    }
}

void WaveManager::setChallengeWaves(int stageIndex) {
    const int bonus = std::max(0, stageIndex) * 3;
    waves = {
        {12 + bonus, 2.15f, 4, 8, 16, 0},
        {15 + bonus, 1.80f, 5, 10, 20, 4},
        {18 + bonus, 1.58f, 6, 14, 22, 8},
        {21 + bonus, 1.36f, 7, 18, 24, 12},
    };
}

void WaveManager::setInfiniteWaves(int stageIndex) {
    const int tier = std::max(0, stageIndex);
    const int earlyTierBonus = tier * 2;
    const int lateTierBonus = tier * 3;
    const int fastWeight = std::min(28, 10 + tier * 2);
    const int heavyWeight = std::min(30, 10 + tier * 2);
    const int adaptiveWeight = std::min(24, 2 + tier * 2);
    const int aliveBonus = tier / 2;

    waves = {
        {8 + earlyTierBonus, std::max(1.35f, 2.30f - tier * 0.08f), 3 + aliveBonus, fastWeight, heavyWeight, adaptiveWeight},
        {10 + earlyTierBonus, std::max(1.10f, 1.95f - tier * 0.075f), 4 + aliveBonus, fastWeight + 2, heavyWeight + 2, adaptiveWeight + 2},
        {12 + lateTierBonus, std::max(0.92f, 1.65f - tier * 0.07f), 5 + aliveBonus, fastWeight + 4, heavyWeight + 4, adaptiveWeight + 5},
    };
}

void WaveManager::configurePreparationValues(GameMode mode, int stageIndex) {
    switch (mode) {
        case GameMode::Story:
            initialPreparationTime = std::max(9.f, 14.f - stageIndex * 2.f);
            betweenWavePreparationTime = std::max(4.f, 7.f - stageIndex * 1.f);
            initialPreparationCreditsValue = stageIndex == 0 ? 28 : (stageIndex == 1 ? 22 : 18);
            betweenWavePreparationCreditsValue = stageIndex == 0 ? 14 : (stageIndex == 1 ? 12 : 10);
            break;
        case GameMode::Challenge:
            initialPreparationTime = 12.f;
            betweenWavePreparationTime = 5.f;
            initialPreparationCreditsValue = 24;
            betweenWavePreparationCreditsValue = 10;
            break;
        case GameMode::Infinite:
            initialPreparationTime = std::max(8.f, 12.f - stageIndex * 0.8f);
            betweenWavePreparationTime = std::max(4.f, 6.f - stageIndex * 0.5f);
            initialPreparationCreditsValue = stageIndex <= 1 ? 24 : (stageIndex <= 4 ? 20 : 16);
            betweenWavePreparationCreditsValue = stageIndex <= 2 ? 13 : (stageIndex <= 6 ? 11 : 9);
            break;
    }
}
