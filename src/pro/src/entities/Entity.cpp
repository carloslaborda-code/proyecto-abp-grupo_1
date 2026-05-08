#include "entities/Entity.h"

Entity::Entity(Layer entityLayer)
    : layer(entityLayer), previousPosition(0.f, 0.f), position(0.f, 0.f), size(0.f, 0.f), alive(true) {
}

Layer Entity::getLayer() const {
    return layer;
}

const sf::Vector2f& Entity::getPosition() const {
    return position;
}

const sf::Vector2f& Entity::getSize() const {
    return size;
}

bool Entity::isAlive() const {
    return alive;
}

sf::Vector2f Entity::interpolatedPosition(float alpha) const {
    return {
        previousPosition.x + (position.x - previousPosition.x) * alpha,
        previousPosition.y + (position.y - previousPosition.y) * alpha
    };
}

void Entity::setPosition(const sf::Vector2f& newPosition) {
    position = newPosition;
}

void Entity::setSize(const sf::Vector2f& newSize) {
    size = newSize;
}

void Entity::setAlive(bool aliveState) {
    alive = aliveState;
}

void Entity::snapshotPosition() {
    previousPosition = position;
}

void Entity::syncPosition() {
    previousPosition = position;
}

AABB Entity::getAABB() const {
    return {sf::FloatRect(position.x, position.y, size.x, size.y)};
}
