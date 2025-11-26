#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <SDL2pp/SDL2pp.hh>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include "../common/constants.h"
#include "camera.h"
#include "car.h"
#include "minimap.h"
#include "checkpoint.h"
#include "audio_manager.h"

using namespace SDL2pp;

struct UpperLayerRect
{
    float x;
    float y;
    float w;
    float h;
};

class GameRenderer
{
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
    std::vector<UpperLayerRect> upperRects;
    std::unique_ptr<AudioManager> audioManager;
    std::map<int, CarPosition> previousCarPositions;

    // --- Rendering ---
    void renderBackground();
    void renderCar(Car &car);
    void renderUpperLayer();
    void renderCheckpoints();

    void updateMainCar(const CarPosition &position);
    void updateCheckpoints(const std::vector<Position> &positions);

    void updateOtherCars(const std::map<int, std::pair<CarPosition, int>> &positions);

    std::set<int> computeNearestCars(
        const std::map<int, std::pair<CarPosition, int>> &positions,
        const CarPosition &mainPos);

    void updateOrCreateCars(
        const std::map<int, std::pair<CarPosition, int>> &positions,
        const std::set<int> &nearest4,
        const CarPosition &mainPos);

    void cleanupRemovedCars(
        const std::map<int, std::pair<CarPosition, int>> &positions,
        const CarPosition &mainPos);

    void updateEngineStates(const std::set<int> &nearest4);

public:
    GameRenderer(const char *windowTitle,
                 int windowWidth,
                 int windowHeight,
                 int mapId = 1,
                 const std::string &tiledJsonPath = "");

    void render(const CarPosition &mainCarPos,
                int mainCarTypeId,
                const std::map<int, std::pair<CarPosition, int>> &otherCarPositions,
                const std::vector<Position> &next_checkpoints);

    void setMainCarType(int typeId)
    {
        if (mainCar)
            mainCar->setCarType(typeId);
    }

    AudioManager* getAudioManager() { return audioManager.get(); }
};

#endif // GAME_RENDERER_H
