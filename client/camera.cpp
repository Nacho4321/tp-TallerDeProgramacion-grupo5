#include "camera.h"
#include <algorithm>

Camera::Camera(int screenW, int screenH, int worldW, int worldH)
    : position(0, 0),
      screenWidth(screenW),
      screenHeight(screenH),
      worldWidth(worldW),
      worldHeight(worldH)
{
}

void Camera::update(const CarPosition& target) {
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

void Camera::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void Camera::setWorldSize(int width, int height) {
    worldWidth = width;
    worldHeight = height;
}

Vector2 Camera::getPosition() const {
    return position;
}
