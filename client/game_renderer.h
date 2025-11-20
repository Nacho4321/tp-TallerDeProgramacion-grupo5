#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H
#include <SDL2pp/SDL2pp.hh>
#include <vector>
#include <memory>
#include "camera.h"
#include "car.h"
#include "minimap.h"  
#include "../common/Event.h"
#include "../common/constants.h"
#include "checkpoint.h"

using namespace SDL2pp;

static std::unordered_map<int, std::string> mapsDataPaths = {
    {1, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Liberty City.png"},
    {2, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - San Andreas.png"},
    {3, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Vice City.png"}
};

static std::string CarDataPath = "data/cars/Mobile - Grand Theft Auto 4 - Miscellaneous - Cars.png";

class GameRenderer {
private:
    SDL sdl;
    Window window;
    Renderer renderer;
    Texture carSprites;
    Texture backgroundTexture;
    Camera camera;
    Minimap minimap;  
    std::unique_ptr<Car> mainCar;
    std::vector<Car> otherCars;
    std::vector<Checkpoint> checkpoints;

public:
    GameRenderer(const char* windowTitle, int windowWidth, int windowHeight, int mapId = 1)
        : sdl(SDL_INIT_VIDEO),
          window(windowTitle,
                 SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                 windowWidth, windowHeight,
                 SDL_WINDOW_RESIZABLE),
          renderer(window, -1, SDL_RENDERER_ACCELERATED),
          carSprites(renderer, [] {
              SDL2pp::Surface surface(CarDataPath);
              surface.SetColorKey(true, SDL_MapRGB(surface.Get()->format, 0xA3, 0xA3, 0x0D));
              return surface;
          }()),
          backgroundTexture(renderer, Surface(mapsDataPaths.at(mapId))),
          camera(windowWidth, windowHeight, backgroundTexture.GetWidth(), backgroundTexture.GetHeight()),
          minimap(backgroundTexture.GetWidth(), backgroundTexture.GetHeight()), 
          mainCar(std::make_unique<Car>()) {
        
        minimap.initialize(renderer, backgroundTexture);
    }

    void updateMainCar(const CarPosition& position) {
        mainCar->setPosition(position);
    }

    void updateOtherCars(const std::vector<CarPosition>& positions) {
        otherCars.clear();
        for (const auto& pos : positions) {
            otherCars.emplace_back(pos); 
        }
    }

    void render(const CarPosition& mainCarPos, const std::vector<CarPosition>& otherCarPositions,
                const std::vector<Position>& next_checkpoints) {
        updateMainCar(mainCarPos);
        updateOtherCars(otherCarPositions);
        updateCheckpoints(next_checkpoints);

        camera.setScreenSize(renderer.GetOutputWidth(), renderer.GetOutputHeight());
        camera.update(mainCar->getPosition());
        renderer.Clear();
        renderBackground();
        renderCar(*mainCar);
        for (const auto& car : otherCars) {
            renderCar(car);
        }

        renderCheckpoints();

        minimap.render(renderer, *mainCar, otherCars, next_checkpoints); 
        renderer.Present();
    }

    int getBackgroundWidth() const {
        return backgroundTexture.GetWidth();
    }

    int getBackgroundHeight() const {
        return backgroundTexture.GetHeight();
    }

private:
    void renderBackground() {
        Vector2 camPos = camera.getPosition();
        renderer.Copy(
            backgroundTexture,
            NullOpt,
            Rect((int)-camPos.x, (int)-camPos.y, 
                 backgroundTexture.GetWidth(), 
                 backgroundTexture.GetHeight())
        );
    }

    void renderCar(const Car& car) {
        Vector2 camPos = camera.getPosition();
        const CarPosition& pos = car.getPosition();
        SDL_Rect sprite = car.getSprite();
        int carScreenX = (int)(pos.x - camPos.x);
        int carScreenY = (int)(pos.y - camPos.y);
        renderer.Copy(
            carSprites,
            Rect(sprite.x, sprite.y, sprite.w, sprite.h),
            Rect(carScreenX, carScreenY, sprite.w, sprite.h)
        );
    }

   void updateCheckpoints(const std::vector<Position>& positions) {
        checkpoints.clear();
        for (size_t i = 0; i < positions.size(); ++i) {
            Checkpoint cp(positions[i], i, i < positions.size() - 1);
            if (i < positions.size() - 1) {
                cp.setNextCheckpoint(positions[i + 1]);
            }
            checkpoints.push_back(cp);
        }
    }

    void renderCheckpoints() {
        Vector2 camPos = camera.getPosition();
        for (const auto& checkpoint : checkpoints) {
            checkpoint.render(renderer, camPos);
        }
        
        // Render screen indicator for the first checkpoint (if it exists)
        if (!checkpoints.empty()) {
            Checkpoint::renderScreenIndicator(
                renderer,
                checkpoints[0].getPosition(),
                camPos,
                renderer.GetOutputWidth(), 
                renderer.GetOutputHeight()
            );
        }
    }
};
#endif // GAME_RENDERER_H