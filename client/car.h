#ifndef CAR_H
#define CAR_H

#include <SDL2/SDL.h>
#include <cmath>
#include <array>

#include "carsprites.h"

struct CarPosition
{
    double x;
    double y;
    double directionX;
    double directionY;
    bool on_bridge;
};

class Car
{
public:
    static constexpr int FLASH_DURATION = 6;  // ~100ms at 60fps

private:
    CarPosition position;
    int spriteIndex;
    double renderedAngle;
    int carType;
    bool exploding;
    int explosionFrame;
    int explosionFrameDelay;

    bool flashing;
    int flashFrame;

    float hp = 100.0f;
    float maxHP = 100.0f;

public:

    bool previousIsStopping = false;

    Car(int carTypeId = 0);

    Car(const CarPosition &pos, int carTypeId = 0);

    void setPosition(const CarPosition &pos);
    const CarPosition &getPosition() const;

    SDL_Rect getSprite() const;

    void startExplosion();
    void updateExplosion();
    bool isExploding() const;
    bool isExplosionComplete() const;
    void stopExplosion();

    void startFlash();
    void updateFlash();
    bool isFlashing() const { return flashing; }
    bool isFlashComplete() const;
    void stopFlash();
    int getFlashFrame() const { return flashFrame; }

    void setCarType(int newType);

    void setHP(float newHP);
    void setMaxHP(float newMaxHP);
    float getHP() const { return hp; }
    float getMaxHP() const { return maxHP; }
    float getHPPercentage() const;

private:
    void updateSpriteIndex();
};

#endif
