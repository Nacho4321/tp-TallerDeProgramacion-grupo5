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
    int carType;

public:
    Car(int carTypeId = 0);

    Car(const CarPosition& pos, int carTypeId = 0);

    void setPosition(const CarPosition& pos);
    const CarPosition& getPosition() const;

    SDL_Rect getSprite() const;

private:
    void updateSpriteIndex();
};

#endif
