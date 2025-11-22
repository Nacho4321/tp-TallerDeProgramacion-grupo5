#include "game_renderer.h"

static std::unordered_map<int, std::string> mapsDataPaths = {
    {1, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Liberty City.png"},
    {2, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - San Andreas.png"},
    {3, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Vice City.png"}
};

static std::string CarDataPath = "data/cars/Mobile - Grand Theft Auto 4 - Miscellaneous - Cars.png";
static std::string ExplosionDataPath = "data/cars/explosion_pixelfied.png";

GameRenderer::GameRenderer(const char* windowTitle, int windowWidth, int windowHeight, int mapId)
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
      explosionSprites(renderer, [] {
          SDL2pp::Surface surface(ExplosionDataPath);
          surface.SetColorKey(true, SDL_MapRGB(surface.Get()->format, 0xFF, 0xFF, 0xFF));
          return surface;
      }()),
      backgroundTexture(renderer, Surface(mapsDataPaths.at(mapId))),
      camera(LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, backgroundTexture.GetWidth(), backgroundTexture.GetHeight()),
      minimap(backgroundTexture.GetWidth(), backgroundTexture.GetHeight()),
      mainCar(std::make_unique<Car>()),
      logicalWidth(LOGICAL_SCREEN_WIDTH),
      logicalHeight(LOGICAL_SCREEN_HEIGHT) {

    // Set up logical rendering size for aspect ratio preservation with letterboxing
    SDL_RenderSetLogicalSize(renderer.Get(), logicalWidth, logicalHeight);
    minimap.initialize(renderer, backgroundTexture);
}

void GameRenderer::updateMainCar(const CarPosition& position) {
    mainCar->setPosition(position);
}

void GameRenderer::updateOtherCars(const std::map<int, CarPosition>& positions) {
    for (const auto& [id, pos] : positions) {
        auto it = otherCars.find(id);
        if (it != otherCars.end()) {
            it->second.setPosition(pos);
        } else {
            otherCars.emplace(id, Car(pos));
        }
    }

    for (auto it = otherCars.begin(); it != otherCars.end(); ) {
        if (positions.find(it->first) == positions.end()) {
            // Car is no longer in the positions map
            if (!it->second.isExploding()) {
                // Start explosion animation
                it->second.startExplosion();
                ++it;
            } else if (it->second.isExplosionComplete()) {
                // Explosion animation is complete, remove the car
                it = otherCars.erase(it);
            } else {
                // Explosion is in progress, keep the car
                ++it;
            }
        } else {
            ++it;
        }
    }
}

void GameRenderer::render(const CarPosition& mainCarPos, const std::map<int, CarPosition>& otherCarPositions,
            const std::vector<Position>& next_checkpoints) {
    updateMainCar(mainCarPos);
    updateOtherCars(otherCarPositions);
    updateCheckpoints(next_checkpoints);

    camera.setScreenSize(logicalWidth, logicalHeight);
    camera.update(mainCar->getPosition());
    renderer.SetDrawColor(0, 0, 0, 255);  
    renderer.Clear();
    renderBackground();
    renderCar(*mainCar);

    renderCheckpoints();

    std::vector<Car> otherCarsVec;
    for (auto& [id, car] : otherCars) {
        renderCar(car);
        otherCarsVec.push_back(car);
    }

    minimap.render(renderer, *mainCar, otherCarsVec, next_checkpoints, logicalWidth, logicalHeight);

    renderer.Present();
}

void GameRenderer::renderBackground() {
    Vector2 camPos = camera.getPosition();
    renderer.Copy(
        backgroundTexture,
        NullOpt,
        Rect((int)-camPos.x, (int)-camPos.y, 
             backgroundTexture.GetWidth(), 
             backgroundTexture.GetHeight())
    );
}

void GameRenderer::renderCar(Car& car) {
    Vector2 camPos = camera.getPosition();
    const CarPosition& pos = car.getPosition();
    SDL_Rect sprite = car.getSprite();
    int carScreenX = (int)(pos.x - camPos.x);
    int carScreenY = (int)(pos.y - camPos.y);

    if (car.isExploding()) {
        car.updateExplosion();
        renderer.Copy(
            explosionSprites,
            Rect(sprite.x, sprite.y, sprite.w, sprite.h),
            Rect(carScreenX, carScreenY, sprite.w, sprite.h)
        );
    } else {
        renderer.Copy(
            carSprites,
            Rect(sprite.x, sprite.y, sprite.w, sprite.h),
            Rect(carScreenX, carScreenY, sprite.w, sprite.h)
        );
    }
}

void GameRenderer::updateCheckpoints(const std::vector<Position>& positions) {
    checkpoints.clear();
    for (size_t i = 0; i < positions.size(); ++i) {
        Checkpoint cp(positions[i], i, i < positions.size() - 1);
        if (i < positions.size() - 1) {
            cp.setNextCheckpoint(positions[i + 1]);
        }
        checkpoints.push_back(cp);
    }
}

void GameRenderer::renderCheckpoints() {
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
            logicalWidth,
            logicalHeight
        );
    }
}