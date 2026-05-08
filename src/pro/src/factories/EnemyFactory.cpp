#include "factories/EnemyFactory.h"

Enemy EnemyFactory::createEnemy(EnemyType type,
                                int laneId,
                                const Level& level,
                                float standardSpeed,
                                float fastSpeed,
                                float heavySpeed,
                                float adaptiveSpeed) {
    Enemy enemy;
    enemy.type = type;
    enemy.laneId = laneId;
    if (type == EnemyType::Fast) {
        enemy.setSize({32.f, 22.f});
    } else if (type == EnemyType::Heavy) {
        enemy.setSize({38.f, 26.f});
    } else if (type == EnemyType::Adaptive) {
        enemy.setSize({34.f, 24.f});
    } else {
        enemy.setSize({34.f, 24.f});
    }

    const sf::Vector2f spawnCenter = level.laneSpawnCenter(laneId);
    enemy.setPosition({spawnCenter.x - enemy.getSize().x / 2.f, spawnCenter.y - enemy.getSize().y / 2.f});

    if (type == EnemyType::Heavy) {
        enemy.hp = 225;
    } else if (type == EnemyType::Fast) {
        enemy.hp = 105;
    } else if (type == EnemyType::Adaptive) {
        enemy.hp = 125;
    } else {
        enemy.hp = 145;
    }

    enemy.baseSpeed = standardSpeed;
    if (type == EnemyType::Fast) {
        enemy.baseSpeed = fastSpeed;
    } else if (type == EnemyType::Heavy) {
        enemy.baseSpeed = heavySpeed;
    } else if (type == EnemyType::Adaptive) {
        enemy.baseSpeed = adaptiveSpeed;
    }

    enemy.velocity = {-enemy.baseSpeed, 0.f};
    enemy.syncPosition();
    return enemy;
}
