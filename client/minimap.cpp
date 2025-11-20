// minimap.cpp
#include "minimap.h"
#include <algorithm>

Minimap::Minimap(int worldWidth, int worldHeight, int displayWidth, 
                 int displayHeight, int borderPadding, float zoom)
    : mapWidth(worldWidth),
      mapHeight(worldHeight),
      minimapWidth(displayWidth),
      minimapHeight(displayHeight),
      padding(borderPadding),
      zoomLevel(zoom) {
    scale = calculateScale();
    
    int scaleFactor = 3;
    cacheTextureWidth = minimapWidth * scaleFactor;
    cacheTextureHeight = minimapHeight * scaleFactor;
}

void Minimap::initialize(Renderer& renderer, Texture& backgroundTexture) {
    int originalWidth = mapWidth;
    int originalHeight = mapHeight;
    
    std::vector<std::unique_ptr<Texture>> mipLevels;
    
    int currentWidth = originalWidth;
    int currentHeight = originalHeight;
    
    while (currentWidth > cacheTextureWidth || currentHeight > cacheTextureHeight) {
        currentWidth = std::max((int)(currentWidth / 1.5f), cacheTextureWidth);
        currentHeight = std::max((int)(currentHeight / 1.5f), cacheTextureHeight);
        
        auto mipTexture = std::make_unique<Texture>(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            currentWidth,
            currentHeight
        );
        
        SDL_SetTextureScaleMode(mipTexture->Get(), SDL_ScaleModeLinear);
        mipLevels.push_back(std::move(mipTexture));
    }
    
    Texture* sourceTexture = &backgroundTexture;
    
    for (auto& mipTexture : mipLevels) {
        renderer.SetTarget(*mipTexture);
        renderer.Clear();
        renderer.Copy(*sourceTexture, NullOpt, 
                    Rect(0, 0, mipTexture->GetWidth(), mipTexture->GetHeight()));
        sourceTexture = mipTexture.get();
    }
    
    minimapTexture = std::make_unique<Texture>(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        cacheTextureWidth,
        cacheTextureHeight
    );
    
    SDL_SetTextureScaleMode(minimapTexture->Get(), SDL_ScaleModeLinear);
    
    renderer.SetTarget(*minimapTexture);
    renderer.Clear();
    renderer.Copy(*sourceTexture, NullOpt, 
                Rect(0, 0, cacheTextureWidth, cacheTextureHeight));
    
    renderer.SetTarget();
}

void Minimap::render(Renderer& renderer, const Car& mainCar, 
                    const std::vector<Car>& otherCars,
                    const std::vector<Position>& checkpoints) {
    Point minimapPos = calculateMinimapPosition(renderer);
    Rect worldViewRect = calculateWorldViewRect(mainCar.getPosition());
    
    drawMinimapBackground(renderer, minimapPos);
    
    Rect cacheViewRect = scaleRectToCacheTexture(worldViewRect);
    Rect dstRect(minimapPos.x, minimapPos.y, minimapWidth, minimapHeight);
    renderer.Copy(*minimapTexture, cacheViewRect, dstRect);
    
    drawMinimapBorder(renderer, minimapPos);
    drawCheckpoints(renderer, checkpoints, minimapPos, worldViewRect);
    drawCars(renderer, mainCar, otherCars, minimapPos, worldViewRect);
}

float Minimap::calculateScale() const {
    float scaleX = (float)minimapWidth / mapWidth;
    float scaleY = (float)minimapHeight / mapHeight;
    return std::min(scaleX, scaleY);
}

Point Minimap::calculateMinimapPosition(const Renderer& renderer) const {
    int screenWidth = renderer.GetOutputWidth();
    int screenHeight = renderer.GetOutputHeight();
    return Point(
        screenWidth - minimapWidth - padding,
        screenHeight - minimapHeight - padding
    );
}

void Minimap::drawMinimapBackground(Renderer& renderer, const Point& pos) const {
    renderer.SetDrawBlendMode(SDL_BLENDMODE_BLEND);
    renderer.SetDrawColor(0, 0, 0, 180);
    renderer.FillRect(Rect(pos.x, pos.y, minimapWidth, minimapHeight));
}

Rect Minimap::calculateWorldViewRect(const CarPosition& carPos) const {
    float worldViewWidth = mapWidth / zoomLevel;
    float worldViewHeight = mapHeight / zoomLevel;
    
    float worldX = carPos.x - worldViewWidth / 2;
    float worldY = carPos.y - worldViewHeight / 2;
    
    worldX = std::max(0.0f, std::min(worldX, mapWidth - worldViewWidth));
    worldY = std::max(0.0f, std::min(worldY, mapHeight - worldViewHeight));
    
    return Rect(
        (int)std::round(worldX), 
        (int)std::round(worldY), 
        (int)std::round(worldViewWidth), 
        (int)std::round(worldViewHeight)
    );
}

Rect Minimap::scaleRectToCacheTexture(const Rect& worldRect) const {
    float scaleX = (float)cacheTextureWidth / mapWidth;
    float scaleY = (float)cacheTextureHeight / mapHeight;
    
    return Rect(
        (int)std::round(worldRect.x * scaleX),
        (int)std::round(worldRect.y * scaleY),
        (int)std::round(worldRect.w * scaleX),
        (int)std::round(worldRect.h * scaleY)
    );
}

void Minimap::drawMinimapBorder(Renderer& renderer, const Point& pos) const {
    renderer.SetDrawColor(255, 255, 255, 255);
    renderer.DrawRect(Rect(pos.x, pos.y, minimapWidth, minimapHeight));
}

void Minimap::drawCheckpoints(Renderer& renderer, const std::vector<Position>& checkpoints,
                             const Point& minimapPos, const Rect& worldViewRect) const {
    for (size_t i = 0; i < checkpoints.size(); ++i) {
        drawCheckpointDot(renderer, checkpoints[i], i, minimapPos, worldViewRect);
    }
}

void Minimap::drawCheckpointDot(Renderer& renderer, const Position& pos, int order,
                               const Point& minimapPos, const Rect& worldViewRect) const {
    float relativeX = pos.new_X - worldViewRect.x;
    float relativeY = pos.new_Y - worldViewRect.y;
    
    if (relativeX < 0 || relativeX > worldViewRect.w || relativeY < 0 || relativeY > worldViewRect.h) {
        return;  
    }
    
    float scaleX = (float)minimapWidth / worldViewRect.w;
    float scaleY = (float)minimapHeight / worldViewRect.h;
    
    int dotX = minimapPos.x + (int)(relativeX * scaleX);
    int dotY = minimapPos.y + (int)(relativeY * scaleY);
    
    Uint8 alpha = 255 - order * 100; 
    
    renderer.SetDrawBlendMode(SDL_BLENDMODE_BLEND);
    renderer.SetDrawColor(255, 220, 0, alpha);

    int radius = 4.5 - order;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx*dx + dy*dy <= radius*radius) {
                renderer.DrawPoint(dotX + dx, dotY + dy);
            }
        }
    }
}

void Minimap::drawCars(Renderer& renderer, const Car& mainCar, 
                      const std::vector<Car>& otherCars, const Point& minimapPos,
                      const Rect& worldViewRect) const {
    // Red for other cars
    renderer.SetDrawColor(255, 0, 0, 255);
    for (const auto& car : otherCars) {
        drawCarDot(renderer, car.getPosition(), minimapPos, worldViewRect);
    }
    
    // Green for main car
    renderer.SetDrawColor(0, 255, 0, 255);
    drawCarDot(renderer, mainCar.getPosition(), minimapPos, worldViewRect);
}

void Minimap::drawCarDot(Renderer& renderer, const CarPosition& pos, 
                        const Point& minimapPos, const Rect& worldViewRect) const {
    float relativeX = pos.x - worldViewRect.x;
    float relativeY = pos.y - worldViewRect.y;
    
    float scaleX = (float)minimapWidth / worldViewRect.w;
    float scaleY = (float)minimapHeight / worldViewRect.h;
    
    int dotX = minimapPos.x + (int)(relativeX * scaleX);
    int dotY = minimapPos.y + (int)(relativeY * scaleY);
    int dotSize = 4;
    
    renderer.FillRect(Rect(dotX - dotSize / 2, dotY - dotSize / 2, dotSize, dotSize));
}