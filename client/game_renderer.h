#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <SDL2pp/SDL2pp.hh>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <chrono>
#include <algorithm>
#include <array>
#include "../common/constants.h"
#include "../common/messages.h"
#include "camera.h"
#include "car.h"
#include "minimap.h"
#include "checkpoint.h"
#include "audio_manager.h"
#include "results_screen.h"

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
    std::map<int, Car> otherCars;
    std::vector<Checkpoint> checkpoints;
    int logicalWidth;
    int logicalHeight;
    std::vector<UpperLayerRect> upperRects;
    std::unique_ptr<AudioManager> audioManager;
    std::map<int, CarPosition> previousCarPositions;
    std::unique_ptr<ResultsScreen> resultsScreen;

    static constexpr int HP_BAR_WIDTH = 20;
    static constexpr int HP_BAR_HEIGHT = 6;
    static constexpr int HP_BAR_OFFSET_Y = 3;

    // --- Rendering ---
    void renderBackground();
    void renderCar(Car &car);
    void renderHPBar(const Car& car, int carScreenX, int carScreenY, int spriteWidth, int spriteHeight);
    void renderUpperLayer();
    void renderCheckpoints();
    void updateMainCar(const CarPosition &position, bool collisionFlag, bool isStopping, float hp);
    void updateCheckpoints(const std::vector<Position> &positions);

    void updateOtherCars(const std::map<int, std::pair<CarPosition, int>> &positions,
                         const std::map<int, bool> &collisionFlags,
                         const std::map<int, bool> &isStoppingFlags);

    std::set<int> computeNearestCars(
        const std::map<int, std::pair<CarPosition, int>> &positions,
        const CarPosition &mainPos);

    void updateOrCreateCars(
        const std::map<int, std::pair<CarPosition, int>> &positions,
        const std::map<int, bool> &collisionFlags,
        const std::map<int, bool> &isStoppingFlags,
        const CarPosition &mainPos);

    void cleanupRemovedCars(
        const std::map<int, std::pair<CarPosition, int>> &positions,
        const CarPosition &mainPos);

public:
    std::unique_ptr<Car> mainCar;

    GameRenderer(const char *windowTitle,
                 int windowWidth,
                 int windowHeight,
                 int mapId = 1,
                 const std::string &tiledJsonPath = "");

    void render(const CarPosition &mainCarPos,
                int mainCarTypeId,
                const std::map<int, std::pair<CarPosition, int>> &otherCarPositions,
                const std::vector<Position> &next_checkpoints,
                bool mainCarCollisionFlag,
                bool mainCarIsStopping,
                float mainCarHP,
                const std::map<int, bool> &otherCarsCollisionFlags,
                const std::map<int, bool> &otherCarsIsStoppingFlags
                );

    void setMainCarType(int typeId)
    {
        if (mainCar)
            mainCar->setCarType(typeId);
    }

    void startCountDown();
    void triggerPlayerDeath();
    void completePlayerDeathTransition();
    void showResults(const std::vector<ServerMessage::PlayerRaceTime>& raceTimes,
                     const std::vector<ServerMessage::PlayerTotalTime>& totalTimes,
                     int32_t mainPlayerId,
                     uint8_t upgrade_speed,
                     uint8_t upgrade_acceleration,
                     uint8_t upgrade_handling,
                     uint8_t upgrade_durability);
    void hideResults();
    void winSound();

    void updateResultsUpgrades(uint8_t upgrade_speed,
                               uint8_t upgrade_acceleration,
                               uint8_t upgrade_handling,
                               uint8_t upgrade_durability);

    AudioManager* getAudioManager() { return audioManager.get(); }
};

#endif // GAME_RENDERER_H
