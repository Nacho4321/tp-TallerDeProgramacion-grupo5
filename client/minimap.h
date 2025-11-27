#ifndef MINIMAP_H
#define MINIMAP_H
#include <SDL2pp/SDL2pp.hh>
#include "camera.h"
#include "car.h"
#include "../common/position.h"
#include <vector>
#include <memory>

using namespace SDL2pp;

class Minimap {
private:
    int mapWidth;
    int mapHeight;
    int minimapWidth;
    int minimapHeight;
    int padding;
    float scale;
    float zoomLevel;
    std::unique_ptr<Texture> minimapTexture; 
    int cacheTextureWidth;  
    int cacheTextureHeight;

public:
    Minimap(int worldWidth, int worldHeight, int displayWidth = 150, 
            int displayHeight = 150, int borderPadding = 10, float zoom = 2.0f);

    void initialize(Renderer& renderer, Texture& backgroundTexture);
    
    void render(Renderer& renderer, const Car& mainCar,
                const std::vector<Car>& otherCars,
                const std::vector<Position>& checkpoints,
                int logicalScreenWidth, int logicalScreenHeight);

private:
    float calculateScale() const;
    Point calculateMinimapPosition(int logicalScreenWidth, int logicalScreenHeight) const;
    void drawMinimapBackground(Renderer& renderer, const Point& pos) const;
    Rect calculateWorldViewRect(const CarPosition& carPos) const;
    Rect scaleRectToCacheTexture(const Rect& worldRect) const;
    void drawMinimapBorder(Renderer& renderer, const Point& pos) const;
    void drawCars(Renderer& renderer, const Car& mainCar, 
                  const std::vector<Car>& otherCars, const Point& minimapPos,
                  const Rect& worldViewRect) const;
    void drawCarDot(Renderer& renderer, const CarPosition& pos, 
                    const Point& minimapPos, const Rect& worldViewRect) const;
    void drawCheckpoints(Renderer& renderer, const std::vector<Position>& checkpoints,
                        const Point& minimapPos, const Rect& worldViewRect) const;
    void drawCheckpointDot(Renderer& renderer, const Position& pos, int order,
                          const Point& minimapPos, const Rect& worldViewRect) const;
};

#endif 