#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <SDL2pp/SDL2pp.hh>
#include <vector>
#include <memory>
#include <map>
#include "camera.h"
#include "car.h"
#include "minimap.h"  
#include "../common/Event.h"
#include "../common/constants.h"
#include "checkpoint.h"

using namespace SDL2pp;

class GameRenderer {
private:
    SDL sdl;
    Window window;
    Renderer renderer;
    Texture carSprites;
    Texture explosionSprites;
    Texture backgroundTexture;
    Camera camera;
    Minimap minimap;
    std::unique_ptr<Car> mainCar;
    std::map<int, Car> otherCars;
    std::vector<Checkpoint> checkpoints;
    int logicalWidth;
    int logicalHeight;

    void renderBackground();
    void renderCar(Car& car);
    void updateCheckpoints(const std::vector<Position>& positions);
    void renderCheckpoints();
    void updateMainCar(const CarPosition& position);
    void updateOtherCars(const std::map<int, std::pair<CarPosition,int>>& positions);


public:
    GameRenderer(const char* windowTitle, int windowWidth, int windowHeight, int mapId = 1);

    void render(const CarPosition& mainCarPos, int mainCarTypeId, const std::map<int, std::pair<CarPosition,int>>& otherCarPositions,
                const std::vector<Position>& next_checkpoints);

    void setMainCarType(int typeId) { if (mainCar) mainCar->setCarType(typeId); }
};

#endif // GAME_RENDERER_H