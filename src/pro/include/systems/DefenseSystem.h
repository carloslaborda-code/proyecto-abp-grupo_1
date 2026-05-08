#pragma once

#include <functional>
#include <vector>

#include "entities/Defense.h"
#include "entities/Enemy.h"
#include "entities/Projectile.h"

class DifficultySystem;
class EconomySystem;

struct DefenseSystemConfig {
    float firewallProjectileRange = 0.f;
    float empTriggerDistance = 0.f;
    float empBlastDistance = 0.f;
    float empPlacementDelay = 0.f;
    float empTriggeredDisplayDuration = 0.f;
    float defenseProjectileSpeed = 0.f;
    float laneBoostAttackMultiplier = 1.f;
    float laneBoostProjectileSpeedMultiplier = 1.f;
    float laneBoostIncomeMultiplier = 1.f;
    int laneBoostDamageBonus = 0;
    int serverIncome = 0;
};

class DefenseSystem {
public:
    using LaneBoostQuery = std::function<bool(int laneId)>;
    using EnemyDefeatCallback = std::function<void(Enemy&, bool rewardPlayer)>;
    using DefenseResetCallback = std::function<void(Defense&, DefenseType type)>;
    using DefenseVisualRefreshCallback = std::function<void(Defense&)>;
    using SoundEffectCallback = std::function<void()>;

    static void update(std::vector<Defense>& defenses,
                       std::vector<Enemy>& enemies,
                       std::vector<Projectile>& projectiles,
                       EconomySystem& economy,
                       const DifficultySystem& difficulty,
                       float dt,
                       const DefenseSystemConfig& config,
                       const LaneBoostQuery& isLaneBoostActive,
                       const EnemyDefeatCallback& defeatEnemy,
                       const DefenseResetCallback& resetDefense,
                       const DefenseVisualRefreshCallback& refreshDefenseVisual,
                       const SoundEffectCallback& playFirewallShotSound,
                       const SoundEffectCallback& playEmpExplosionSound);
};
