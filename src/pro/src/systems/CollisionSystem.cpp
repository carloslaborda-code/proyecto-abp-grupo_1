#include "systems/CollisionSystem.h"

CollisionSystem::CollisionSystem() {
    for (auto& row : matrix) {
        row.fill(false);
    }

    set(Layer::Projectile, Layer::Enemy, true);
    set(Layer::Enemy, Layer::Defense, true);
    set(Layer::Enemy, Layer::Core, true);
}

bool CollisionSystem::canCollide(Layer a, Layer b) const {
    return matrix[static_cast<int>(a)][static_cast<int>(b)];
}

bool CollisionSystem::intersects(const Entity& a, const Entity& b) const {
    return a.getAABB().rect.intersects(b.getAABB().rect);
}

void CollisionSystem::set(Layer a, Layer b, bool value) {
    matrix[static_cast<int>(a)][static_cast<int>(b)] = value;
    matrix[static_cast<int>(b)][static_cast<int>(a)] = value;
}
