#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
#include "car.h"
#include "vector2.h"

class Camera {
private:
    Vector2 position;      
    int screenWidth;
    int screenHeight;
    int worldWidth;   
    int worldHeight;

public:
    Camera(int screenW, int screenH, int worldW = 0, int worldH = 0)
        : position(0, 0),
          screenWidth(screenW),
          screenHeight(screenH),
          worldWidth(worldW),
          worldHeight(worldH) {}

    void update(const CarPosition& target) {
        Vector2 targetOffset;
        targetOffset.x = target.x - screenWidth / 2.0f;
        targetOffset.y = target.y - screenHeight / 2.0f;
        
        if (worldWidth > 0 && worldHeight > 0) {
            targetOffset.x = std::max(0.0f, targetOffset.x);
            targetOffset.y = std::max(0.0f, targetOffset.y);
            
            targetOffset.x = std::min(targetOffset.x, (float)(worldWidth - screenWidth));
            targetOffset.y = std::min(targetOffset.y, (float)(worldHeight - screenHeight));
        }
        
        position.x = targetOffset.x;
        position.y = targetOffset.y;
    }
    
    void setScreenSize(int width, int height) {
        screenWidth = width;
        screenHeight = height;
    }
    
    void setWorldSize(int width, int height) {
        worldWidth = width;
        worldHeight = height;
    }
    
    Vector2 getPosition() const {
        return position;
    }
};

#endif // CAMERA_H