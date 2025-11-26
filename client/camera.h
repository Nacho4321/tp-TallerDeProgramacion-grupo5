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
    Camera(int screenW, int screenH, int worldW = 0, int worldH = 0);

    void update(const CarPosition& target);
    
    void setScreenSize(int width, int height);
    
    void setWorldSize(int width, int height);
    
    Vector2 getPosition() const;
};

#endif // CAMERA_H