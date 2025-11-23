#ifndef CAR_H
#define CAR_H

#include <SDL2/SDL.h>
#include <cmath>
#include <array>

#include "carsprites.h"  

struct CarPosition {
    double x;
    double y;
    double directionX;
    double directionY;
};

class Car {
private:
    CarPosition position;
    int spriteIndex;
    double renderedAngle; // continuous angle used to compute spriteIndex, in radians
    int carType;
    bool exploding;
    int explosionFrame;
    int explosionFrameDelay;

public:
    Car(int carTypeId = 0);

    Car(const CarPosition& pos, int carTypeId = 0);

    void setPosition(const CarPosition& pos);
    const CarPosition& getPosition() const;

    SDL_Rect getSprite() const;

    void startExplosion();
    void updateExplosion();
    bool isExploding() const;
    bool isExplosionComplete() const;

private:
    void updateSpriteIndex();
};

#endif
