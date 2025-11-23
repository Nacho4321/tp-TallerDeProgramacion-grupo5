#include "car.h"
#include <cmath>
#include <iostream>

Car::Car(int carTypeId)
    : position{0, 0, 0, 0}, spriteIndex(0), renderedAngle(0.0), carType(carTypeId), exploding(false), explosionFrame(0), explosionFrameDelay(0)
{}

Car::Car(const CarPosition& pos, int carTypeId)
    : position(pos), spriteIndex(0), renderedAngle(0.0), carType(carTypeId), exploding(false), explosionFrame(0), explosionFrameDelay(0)
{
    // Initialize renderedAngle from initial position if available
    if (position.directionX != 0 || position.directionY != 0) {
        renderedAngle = std::atan2(position.directionY, position.directionX);
        while (renderedAngle < 0) renderedAngle += 2.0 * M_PI;
    } else {
        renderedAngle = 0.0;
    }
    updateSpriteIndex();
}

void Car::setPosition(const CarPosition& pos) {
    // Update stored position and smoothly update renderedAngle toward server-provided orientation
    position = pos;

    // If direction is zero (no direction info), keep previous renderedAngle
    if (position.directionX == 0 && position.directionY == 0) {
        // nothing to update
        return;
    }

    // Compute target angle from direction vector (server provides a forward unit-ish vector)
    double target = std::atan2(position.directionY, position.directionX);

    // Normalize delta to (-pi, pi]
    double delta = target - renderedAngle;
    while (delta <= -M_PI) delta += 2.0 * M_PI;
    while (delta > M_PI) delta -= 2.0 * M_PI;

    // One sprite sector in radians
    const double sector = 2.0 * M_PI / NUM_CAR_SPRITES;
    const double SNAP_THRESHOLD_DEG = 30.0; // if larger than this, snap
    const double SNAP_THRESHOLD_RAD = SNAP_THRESHOLD_DEG * M_PI / 180.0;

    // Compute desired sprite index from target angle and current sprite index from renderedAngle
    double targetDeg = target * 180.0 / M_PI;
    if (targetDeg < 0) targetDeg += 360.0;
    int desiredSprite = (int)std::round(targetDeg / (360.0 / NUM_CAR_SPRITES)) % NUM_CAR_SPRITES;
    double renderedDeg = renderedAngle * 180.0 / M_PI;
    if (renderedDeg < 0) renderedDeg += 360.0;
    int currentSprite = (int)std::round(renderedDeg / (360.0 / NUM_CAR_SPRITES)) % NUM_CAR_SPRITES;

    // minimal sprite distance taking wrap into account
    int total = NUM_CAR_SPRITES;
    int forwardSteps = (desiredSprite - currentSprite + total) % total;
    int backwardSteps = (currentSprite - desiredSprite + total) % total;
    int minSteps = std::min(forwardSteps, backwardSteps);

    // Conditional debug logging
    bool dbg = false;
    const char* env = std::getenv("TALLER_DEBUG_CAR");
    if (env && env[0] != '\0') dbg = true;

    if (dbg) {
        double targDegDbg = target * 180.0 / M_PI;
        double rendDegDbg = renderedAngle * 180.0 / M_PI;
        if (targDegDbg < 0) targDegDbg += 360.0;
        if (rendDegDbg < 0) rendDegDbg += 360.0;
        std::cout << "[CarDbg] pos=(" << position.x << "," << position.y << ") "
                  << "targetDeg=" << targDegDbg << " renderedDeg=" << rendDegDbg
                  << " desiredSprite=" << desiredSprite << " currentSprite=" << currentSprite
                  << " deltaDeg=" << (delta * 180.0 / M_PI) << std::endl;
    }

    if (minSteps > 1 || std::abs(delta) > SNAP_THRESHOLD_RAD) {
        // If more than one sprite step away (or a large jump in radians), snap to avoid slow accumulation/desync
        renderedAngle = target;
        // normalize
        while (renderedAngle < 0) renderedAngle += 2.0 * M_PI;
        while (renderedAngle >= 2.0 * M_PI) renderedAngle -= 2.0 * M_PI;
        spriteIndex = desiredSprite;
        if (dbg) {
            std::cout << "[CarDbg] SNAP to spriteIndex=" << spriteIndex << "\n";
        }
    } else {
        // small change: advance by at most one sector in the shortest direction
        if (delta > 0)
            renderedAngle += std::min(delta, sector);
        else
            renderedAngle -= std::min(-delta, sector);

        // normalize renderedAngle to [0, 2PI)
        while (renderedAngle < 0) renderedAngle += 2.0 * M_PI;
        while (renderedAngle >= 2.0 * M_PI) renderedAngle -= 2.0 * M_PI;

        // Update spriteIndex based on renderedAngle
        double degrees = renderedAngle * 180.0 / M_PI;
        int idx = (int)round(degrees / (360.0 / NUM_CAR_SPRITES)) % NUM_CAR_SPRITES;
        spriteIndex = (idx + NUM_CAR_SPRITES) % NUM_CAR_SPRITES;
        if (dbg) {
            std::cout << "[CarDbg] STEP to spriteIndex=" << spriteIndex << " renderedDeg=" << (renderedAngle * 180.0 / M_PI) << "\n";
        }
    }
}

const CarPosition& Car::getPosition() const {
    return position;
}

SDL_Rect Car::getSprite() const {
    if (exploding) {
        return EXPLOSION_SPRITES[explosionFrame];
    }
    return CAR_SPRITES[carType][spriteIndex];
}

void Car::updateSpriteIndex() {
    if (position.directionX != 0 || position.directionY != 0) {
        double angle = atan2(position.directionY, position.directionX);
        double degrees = angle * 180.0 / M_PI;

        if (degrees < 0) degrees += 360;
        degrees = fmod(degrees, 360);

        int desiredSprite = (int)round(degrees / 22.5) % NUM_CAR_SPRITES;

        // Smooth transition: snap when angle difference is large, otherwise step by 1 towards target.
        if (desiredSprite != spriteIndex) {
            const int total = NUM_CAR_SPRITES;
            // compute current angle represented by spriteIndex
            double currentDegrees = (double)spriteIndex * (360.0 / total);
            // minimal circular difference in degrees
            double diff = degrees - currentDegrees;
            while (diff <= -180.0) diff += 360.0;
            while (diff > 180.0) diff -= 360.0;

            const double SNAP_THRESHOLD_DEG = 30.0; // if rotation larger than this, snap immediately
            if (std::abs(diff) > SNAP_THRESHOLD_DEG) {
                spriteIndex = desiredSprite;
            } else {
                int forward  = (desiredSprite - spriteIndex + total) % total;
                int backward = (spriteIndex - desiredSprite + total) % total;
                if (forward <= backward) {
                    spriteIndex = (spriteIndex + 1) % total;
                } else {
                    spriteIndex = (spriteIndex - 1 + total) % total;
                }
            }
        }
    }
}

void Car::startExplosion() {
    exploding = true;
    explosionFrame = 0;
    explosionFrameDelay = 3;  // Show each frame for n render cycles
}

void Car::updateExplosion() {
    if (exploding && explosionFrame < NUM_EXPLOSION_SPRITES - 1) {
        if (explosionFrameDelay < 3) {
            explosionFrameDelay++;
        } else {
            explosionFrame++;
            explosionFrameDelay = 0;  // Reset delay for next frame
        }
    }
}

bool Car::isExploding() const {
    return exploding;
}

bool Car::isExplosionComplete() const {
    return exploding && explosionFrame >= NUM_EXPLOSION_SPRITES - 1;
}
