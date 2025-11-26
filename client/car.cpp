#include "car.h"
#include <cmath>

Car::Car(int carTypeId)
    : position{0, 0, 0, 0}, spriteIndex(0), renderedAngle(0.0), carType(carTypeId), exploding(false), explosionFrame(0), explosionFrameDelay(0)
{}

Car::Car(const CarPosition& pos, int carTypeId)
    : position(pos), spriteIndex(0), renderedAngle(0.0), carType(carTypeId), exploding(false), explosionFrame(0), explosionFrameDelay(0)
{
    updateSpriteIndex();
}

void Car::setPosition(const CarPosition& pos) {
    position = pos;
    updateSpriteIndex();
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

        spriteIndex = (int)round(degrees / 22.5) % NUM_CAR_SPRITES;
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

void Car::setCarType(int newType) {
    carType = newType;
    spriteIndex = 0;
    updateSpriteIndex();
}
