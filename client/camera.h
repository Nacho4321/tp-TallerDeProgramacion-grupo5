#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
#include "car.h"

struct Vector2 {
    float x;
    float y;
    
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
};

class Camera {
private:
    Vector2 position;      
    int screenWidth;
    int screenHeight;

public:
    Camera(int screenW, int screenH)
        : position(0, 0),
          screenWidth(screenW),
          screenHeight(screenH) {}

    void update(const CarPosition& target) {
        Vector2 targetOffset;
        targetOffset.x = target.x - screenWidth / 2.0f;
        targetOffset.y = target.y - screenHeight / 2.0f;
        
        position.x = targetOffset.x;
        position.y = targetOffset.y;
    }
    
    void setScreenSize(int width, int height) {
        screenWidth = width;
        screenHeight = height;
    }
    
    Vector2 getPosition() const {
        return position;
    }
};

#endif // CAMERA_H
