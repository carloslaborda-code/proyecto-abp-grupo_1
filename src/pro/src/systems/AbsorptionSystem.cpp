#include "systems/AbsorptionSystem.h"

#include <algorithm>

namespace {

std::array<int, 4> defaultThresholds() {
    return {8, 6, 5, 4};
}

}  // namespace

AbsorptionSystem::AbsorptionSystem() {
    reset();
}

void AbsorptionSystem::reset() {
    const auto thresholds = defaultThresholds();
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        tracks[i].progress = 0;
        tracks[i].charges = 0;
        tracks[i].threshold = thresholds[i];
    }
}

void AbsorptionSystem::notifyEnemyDefeated(EnemyType type) {
    Track& track = tracks[indexFor(type)];
    ++track.progress;
    if (track.progress < track.threshold) {
        return;
    }

    track.progress = 0;
    track.charges = std::min(track.charges + 1, 2);
}

void AbsorptionSystem::grantCharge(EnemyType type, int count) {
    Track& track = tracks[indexFor(type)];
    track.progress = 0;
    track.charges = std::min(track.charges + std::max(0, count), 2);
}

void AbsorptionSystem::grantAllCharges(int count) {
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        tracks[i].progress = 0;
        tracks[i].charges = std::min(tracks[i].charges + std::max(0, count), 2);
    }
}

bool AbsorptionSystem::hasCharge(EnemyType type) const {
    return charges(type) > 0;
}

bool AbsorptionSystem::consume(EnemyType type) {
    Track& track = tracks[indexFor(type)];
    if (track.charges <= 0) {
        return false;
    }

    --track.charges;
    return true;
}

int AbsorptionSystem::charges(EnemyType type) const {
    return tracks[indexFor(type)].charges;
}

int AbsorptionSystem::progress(EnemyType type) const {
    return tracks[indexFor(type)].progress;
}

int AbsorptionSystem::threshold(EnemyType type) const {
    return tracks[indexFor(type)].threshold;
}

int AbsorptionSystem::remainingToUnlock(EnemyType type) const {
    const Track& track = tracks[indexFor(type)];
    return std::max(0, track.threshold - track.progress);
}

std::size_t AbsorptionSystem::indexFor(EnemyType type) {
    switch (type) {
        case EnemyType::Standard:
            return 0;
        case EnemyType::Fast:
            return 1;
        case EnemyType::Heavy:
            return 2;
        case EnemyType::Adaptive:
            return 3;
    }

    return 0;
}
