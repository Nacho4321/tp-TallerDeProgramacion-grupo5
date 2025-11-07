#ifndef CAR_H
#define CAR_H

#include <SDL2/SDL.h>
#include <cmath>

static SDL_Rect CAR_SPRITES[] = {
    { 2, 5, 28, 22}, { 32, 3, 31, 25}, { 65, 2, 29, 29},
    { 99, 1, 25, 30}, { 133, 2, 22, 28}, { 164, 1, 25, 30},
    { 194, 2, 29, 29}, { 225, 3, 31, 25}, { 2, 37, 28, 22},
    { 32, 36, 32, 26}, { 66, 33, 29, 29}, { 99, 33, 26, 30},
    { 133, 33, 22, 29}, { 163, 33, 25, 30}, { 193, 33, 29, 29},
    { 224, 36, 31, 25}
};

static const int NUM_CAR_SPRITES = sizeof(CAR_SPRITES) / sizeof(SDL_Rect);

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
    
public:
    Car() : spriteIndex(0) {
        position = {0, 0, 0, 0};
    }
    
    Car(const CarPosition& pos) : position(pos), spriteIndex(0) {
        updateSpriteIndex();
    }
    
    void setPosition(const CarPosition& pos) {
        position = pos;
        updateSpriteIndex();
    }
    
    const CarPosition& getPosition() const {
        return position;
    }
    
    SDL_Rect getSprite() const {
        return CAR_SPRITES[spriteIndex];
    }
    
private:
    void updateSpriteIndex() {
        if (position.directionX != 0 || position.directionY != 0) {
            double angle = atan2(position.directionY, position.directionX);
            double degrees = angle * 180.0 / M_PI;
            
            if (degrees < 0) degrees += 360;
            degrees = fmod(degrees, 360);
            
            spriteIndex = (int)round(degrees / 22.5) % NUM_CAR_SPRITES;
        }
    }
};

#endif // CAR_H