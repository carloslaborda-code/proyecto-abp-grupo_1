#include "systems/DefenseSystem.h"

#include <algorithm>
#include <cmath>

#include "systems/AnimationUtils.h"
#include "systems/DifficultySystem.h"
#include "systems/EconomySystem.h"

namespace {

Projectile makeProjectileFromDefense(const Defense& defense,
                                     float speed,
                                     int damage,
                                     float maxRange,
                                     int targetEnemyId) {
    Projectile projectile;
    projectile.laneId = defense.laneId;
    projectile.setSize({14.f, 6.f});
    projectile.setPosition({
        defense.getPosition().x + defense.getSize().x + 2.f,
        defense.getPosition().y + defense.getSize().y * 0.5f - projectile.getSize().y * 0.5f
    });
    projectile.velocity = {speed, 0.f};
    projectile.damage = damage;
    projectile.targetEnemyId = targetEnemyId;
    projectile.owner = ProjectileOwner::Defense;
    projectile.remainingRange = maxRange;
    projectile.syncPosition();
    return projectile;
}

float serverIncomeFactorForActiveCount(int activeServerCount) {
    if (activeServerCount <= 1) {
        return 1.f;
    }
    if (activeServerCount == 2) {
        return 0.72f;
    }
    if (activeServerCount == 3) {
        return 0.56f;
    }
    if (activeServerCount == 4) {
        return 0.46f;
    }
    return 0.38f;
}

int countActiveServers(const std::vector<Defense>& defenses) {
    int activeServerCount = 0;
    for (const auto& defense : defenses) {
        if (defense.placed && defense.hp > 0.f && defense.type == DefenseType::AuxiliaryServer) {
            ++activeServerCount;
        }
    }
    return activeServerCount;
}

}  // namespace

void DefenseSystem::update(std::vector<Defense>& defenses,
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
                           const SoundEffectCallback& playEmpExplosionSound) {
    const int activeServerCount = countActiveServers(defenses);

    for (auto& defense : defenses) {
        if (!defense.placed || defense.hp <= 0.f) {
            continue;
        }

        updateAnimation(defense.animation, dt);
        const bool laneBoosted = isLaneBoostActive(defense.laneId);
        const float fireRateMultiplier = laneBoosted ? config.laneBoostAttackMultiplier : 1.f;
        const float projectileSpeed =
            config.defenseProjectileSpeed * (laneBoosted ? config.laneBoostProjectileSpeedMultiplier : 1.f);
        const float defenseRange = config.firewallProjectileRange * (laneBoosted ? 1.15f : 1.f);
        const int laneDamageBonus = laneBoosted ? config.laneBoostDamageBonus : 0;
        defense.fireTimer += dt * fireRateMultiplier;
        const int lane = defense.laneId;

        const float defenseMuzzleX = defense.getPosition().x + defense.getSize().x;
        Enemy* lockedFirewallTarget = nullptr;

        if (defense.type == DefenseType::Firewall) {
            if (defense.targetEnemyId >= 0) {
                for (auto& enemy : enemies) {
                    if (!enemy.isAlive() || enemy.id != defense.targetEnemyId || enemy.laneId != lane) {
                        continue;
                    }

                    const float enemyCenterX = enemy.getPosition().x + enemy.getSize().x * 0.5f;
                    const float distance = enemyCenterX - defenseMuzzleX;
                    if (distance >= 0.f && distance <= defenseRange) {
                        lockedFirewallTarget = &enemy;
                    }
                    break;
                }

                if (!lockedFirewallTarget) {
                    defense.targetEnemyId = -1;
                }
            }

            if (defense.targetEnemyId < 0) {
                Enemy* bestTarget = nullptr;
                float bestDistance = 0.f;
                for (auto& enemy : enemies) {
                    if (!enemy.isAlive() || enemy.laneId != lane) {
                        continue;
                    }

                    const float enemyCenterX = enemy.getPosition().x + enemy.getSize().x * 0.5f;
                    const float distance = enemyCenterX - defenseMuzzleX;
                    if (distance < 0.f || distance > defenseRange) {
                        continue;
                    }

                    if (!bestTarget || distance < bestDistance) {
                        bestTarget = &enemy;
                        bestDistance = distance;
                    }
                }

                if (bestTarget) {
                    defense.targetEnemyId = bestTarget->id;
                    lockedFirewallTarget = bestTarget;
                }
            }

            if (lockedFirewallTarget && defense.fireTimer >= defense.fireInterval) {
                defense.fireTimer = 0.f;
                projectiles.push_back(makeProjectileFromDefense(defense,
                                                                projectileSpeed,
                                                                defense.damage + laneDamageBonus,
                                                                defenseRange,
                                                                defense.targetEnemyId));
                playFirewallShotSound();
            }
        }

        if (defense.type == DefenseType::EMP) {
            defense.empPhaseTimer += dt;

            if (defense.empState == EMPState::Placement) {
                if (defense.empPhaseTimer >= config.empPlacementDelay) {
                    defense.empState = EMPState::Armed;
                    defense.empPhaseTimer = 0.f;
                    refreshDefenseVisual(defense);
                }
                continue;
            }

            if (defense.empState == EMPState::Armed) {
                bool armedTrigger = false;
                for (auto& enemy : enemies) {
                    if (!enemy.isAlive() || enemy.laneId != lane) {
                        continue;
                    }

                    const float deltaX = enemy.getPosition().x - defense.getPosition().x;
                    if (deltaX < -enemy.getSize().x || deltaX > config.empTriggerDistance) {
                        continue;
                    }

                    armedTrigger = true;
                    break;
                }

                if (armedTrigger) {
                    for (auto& enemy : enemies) {
                        if (!enemy.isAlive() || enemy.laneId != lane) {
                            continue;
                        }

                        const float distance =
                            std::abs((enemy.getPosition().x + enemy.getSize().x * 0.5f) -
                                     (defense.getPosition().x + defense.getSize().x * 0.5f));
                        if (distance <= config.empBlastDistance) {
                            defeatEnemy(enemy, true);
                        }
                    }

                    playEmpExplosionSound();
                    defense.empState = EMPState::Triggered;
                    defense.empPhaseTimer = 0.f;
                    refreshDefenseVisual(defense);
                }
                continue;
            }

            if (defense.empPhaseTimer < config.empTriggeredDisplayDuration) {
                continue;
            }

            defense.placed = false;
            defense.hp = 0.f;
            resetDefense(defense, DefenseType::EMP);
            continue;
        }

        if (defense.type == DefenseType::AuxiliaryServer && defense.fireTimer >= defense.fireInterval) {
            defense.fireTimer = 0.f;
            const float incomeMultiplier =
                difficulty.resourceMultiplier() *
                serverIncomeFactorForActiveCount(activeServerCount) *
                (laneBoosted ? config.laneBoostIncomeMultiplier : 1.f);
            economy.earn(std::max(1, static_cast<int>(std::round(config.serverIncome * incomeMultiplier))));
        }
    }
}
