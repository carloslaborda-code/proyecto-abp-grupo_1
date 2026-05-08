#pragma once

#include <SFML/Graphics.hpp>

enum class Layer : int {
    Projectile = 0,
    Enemy = 1,
    Defense = 2,
    Core = 3,
    Count = 4
};

struct AABB {
    sf::FloatRect rect;
};

class Entity {
public:
    explicit Entity(Layer entityLayer);
    virtual ~Entity() = default;

    Layer getLayer() const;
    const sf::Vector2f& getPosition() const;
    const sf::Vector2f& getSize() const;
    bool isAlive() const;
    sf::Vector2f interpolatedPosition(float alpha) const;

    void setPosition(const sf::Vector2f& newPosition);
    void setSize(const sf::Vector2f& newSize);
    void setAlive(bool aliveState);
    void snapshotPosition();
    void syncPosition();

    virtual AABB getAABB() const;

protected:
    Layer layer;
    sf::Vector2f previousPosition;
    sf::Vector2f position;
    sf::Vector2f size;
    bool alive;
};
