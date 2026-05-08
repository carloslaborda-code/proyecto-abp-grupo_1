#pragma once

#include <optional>
#include <random>
#include <vector>

#include "core/Config.h"
#include "entities/Enemy.h"

struct SpawnRequest {
    EnemyType type = EnemyType::Standard;
    int laneId = 0;
};

struct WaveUpdateResult {
    std::optional<SpawnRequest> spawn;
    int preparationCredits = 0;
    bool enteredPreparation = false;
};

class WaveManager {
public:
    WaveManager();

    void configure(GameMode mode, int stageIndex);
    WaveUpdateResult update(float dt,
                            int laneCount,
                            int aliveEnemies,
                            float spawnIntervalMultiplier,
                            int defeatedEnemies = 0);
    bool hasStageVictory(int aliveEnemies) const;
    int currentWaveNumber() const;
    int totalWaveCount() const;
    int totalEnemiesInStage() const;
    bool isPreparing() const;
    float preparationTimeRemaining() const;
    int preparationSecondsRemaining() const;
    int initialPreparationCredits() const;

private:
    enum class WavePhase {
        Preparation,
        Spawning,
        Cleanup,
        Completed
    };

    struct WaveDefinition {
        int enemyCount;
        float spawnInterval;
        int maxAliveEnemies;
        int fastWeight;
        int heavyWeight;
        int adaptiveWeight;
    };

    int chooseSpawnLane(int laneCount);
    EnemyType rollEnemyType(const WaveDefinition& wave, int defeatedEnemies);
    WaveDefinition infiniteWeightsForDefeatedEnemies(int defeatedEnemies) const;
    void setStoryStageWaves(int stageIndex);
    void setChallengeWaves(int stageIndex);
    void setInfiniteWaves(int stageIndex);
    void configurePreparationValues(GameMode mode, int stageIndex);

    std::vector<WaveDefinition> waves;
    std::mt19937 rng;
    GameMode currentMode;
    int currentStageIndex;
    int currentWaveIndex;
    int spawnedInWave;
    int lastSpawnLaneId;
    float spawnTimer;
    float preparationTimer;
    float initialPreparationTime;
    float betweenWavePreparationTime;
    int initialPreparationCreditsValue;
    int betweenWavePreparationCreditsValue;
    WavePhase phase;
};
