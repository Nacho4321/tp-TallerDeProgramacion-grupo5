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
    double renderedAngle; 
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

    void setCarType(int newType) { carType = newType; spriteIndex = 0; updateSpriteIndex(); }

private:
    void updateSpriteIndex();
};

#endif
