#pragma once

#include <array>

#include "entities/Entity.h"

class CollisionSystem {
public:
    CollisionSystem();

    bool canCollide(Layer a, Layer b) const;
    bool intersects(const Entity& a, const Entity& b) const;

private:
    void set(Layer a, Layer b, bool value);

    std::array<std::array<bool, static_cast<int>(Layer::Count)>,
               static_cast<int>(Layer::Count)> matrix;
};
