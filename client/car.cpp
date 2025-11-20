#include "car.h"
#include <cmath>

Car::Car(int carTypeId)
    : position{0, 0, 0, 0}, spriteIndex(0), carType(carTypeId)
{}

Car::Car(const CarPosition& pos, int carTypeId)
    : position(pos), spriteIndex(0), carType(carTypeId)
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
    return CAR_SPRITES[carType][spriteIndex];
}

void Car::updateSpriteIndex() {
    if (position.directionX != 0 || position.directionY != 0) {
        double angle = atan2(position.directionY, position.directionX);
        double degrees = angle * 180.0 / M_PI;

        if (degrees < 0) degrees += 360;
        degrees = fmod(degrees, 360);

        int desiredSprite = (int)round(degrees / 22.5) % NUM_CAR_SPRITES;

        // Smooth transition
        if (desiredSprite != spriteIndex) {
            int total = NUM_CAR_SPRITES;
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
