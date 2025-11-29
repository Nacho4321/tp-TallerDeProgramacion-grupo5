#include "game_renderer.h"
#include "game_renderer.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <cmath>
#include <algorithm>

static std::unordered_map<int, std::string> mapsDataPaths = {
    {1, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Liberty City.png"},
    {2, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - San Andreas.png"},
    {3, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Vice City.png"}};

static std::string CarDataPath = "data/cars/Mobile - Grand Theft Auto 4 - Miscellaneous - Cars.png";
static std::string ExplosionDataPath = "data/cars/explosion_pixelfied.png";

GameRenderer::GameRenderer(const char *windowTitle, int windowWidth, int windowHeight, int mapId, const std::string &tiledJsonPath)
    : sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO),
      window(windowTitle,
             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
             windowWidth, windowHeight,
             SDL_WINDOW_RESIZABLE),
      renderer(window, -1, SDL_RENDERER_ACCELERATED),
      carSprites(renderer, []
                 {
          SDL2pp::Surface surface(CarDataPath);
          surface.SetColorKey(true, SDL_MapRGB(surface.Get()->format, 0xA3, 0xA3, 0x0D));
          return surface; }()),
      explosionSprites(renderer, []
                       {
          SDL2pp::Surface surface(ExplosionDataPath);
          surface.SetColorKey(true, SDL_MapRGB(surface.Get()->format, 0xFF, 0xFF, 0xFF));
          return surface; }()),
      backgroundTexture(renderer, Surface(mapsDataPaths.at(mapId))),
      camera(LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, backgroundTexture.GetWidth(), backgroundTexture.GetHeight()),
      minimap(backgroundTexture.GetWidth(), backgroundTexture.GetHeight()),
      logicalWidth(LOGICAL_SCREEN_WIDTH),
      logicalHeight(LOGICAL_SCREEN_HEIGHT),
      mainCar(std::make_unique<Car>())

{
    SDL_RenderSetLogicalSize(renderer.Get(), logicalWidth, logicalHeight);
    minimap.initialize(renderer, backgroundTexture);

    audioManager = std::make_unique<AudioManager>();
    audioManager->playBackgroundMusic("data/music/background_loop.ogg");

    audioManager->startCarEngine(-1, 0, 0, 0, 0, true);

    std::ifstream file(tiledJsonPath);
    nlohmann::json data;
    file >> data;
    for (auto &layer : data["layers"])
    {
        if (layer["type"] != "objectgroup")
            continue;

        std::string lname = layer["name"].get<std::string>();

        // Estas son las capas que DEBEN ir arriba del auto visualmente
        if (lname == "Collisions_Bridge" || lname == "Collisions_under")
        {
            for (auto &obj : layer["objects"])
            {
                UpperLayerRect rect;
                rect.x = obj["x"].get<float>();
                rect.y = obj["y"].get<float>();
                rect.w = obj["width"].get<float>();
                rect.h = obj["height"].get<float>();
                upperRects.push_back(rect);
            }
        }
    }
}

void GameRenderer::updateMainCar(const CarPosition &position, bool collisionFlag)
{
    audioManager->updateCarEngineVolume(-1, 0, 0, 0, 0);
    mainCar->setPosition(position);

    if (collisionFlag && mainCar)
    {
        mainCar->startFlash();

        audioManager->playCollisionSound(position.x, position.y, position.x, position.y);
    }
}

void GameRenderer::updateOtherCars(const std::map<int, std::pair<CarPosition, int>> &positions,
                                   const std::map<int, bool> &collisionFlags)
{
    CarPosition mainPos = mainCar->getPosition();

    auto nearest4 = computeNearestCars(positions, mainPos);
    updateOrCreateCars(positions, collisionFlags, nearest4, mainPos);
    cleanupRemovedCars(positions, mainPos);
    if (audioManager)
        audioManager->stopEnginesExcept(nearest4);
}

std::set<int> GameRenderer::computeNearestCars(
    const std::map<int, std::pair<CarPosition, int>> &positions,
    const CarPosition &mainPos)
{
    std::vector<std::pair<float, int>> distances;

    for (const auto &[id, data] : positions)
    {
        if (id < 0) continue; // skip NPCs
        float dx = data.first.x - mainPos.x;
        float dy = data.first.y - mainPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        distances.push_back({dist, id});
    }

    std::sort(distances.begin(), distances.end());

    std::set<int> nearest4;
    for (size_t i = 0; i < std::min<size_t>(4, distances.size()); ++i)
        nearest4.insert(distances[i].second);

    return nearest4;
}

void GameRenderer::updateOrCreateCars(
    const std::map<int, std::pair<CarPosition, int>> &positions,
    const std::map<int, bool> &collisionFlags,
    const std::set<int> &nearest4,
    const CarPosition &mainPos)
{
    for (const auto &[id, data] : positions)
    {
        const CarPosition &pos = data.first;
        int typeId = data.second;
        bool isNearby = nearest4.count(id) > 0;
        auto it = otherCars.find(id);

        if (it != otherCars.end())
        {
            it->second.setPosition(pos);
            it->second.setCarType(typeId);

            auto collisionIt = collisionFlags.find(id);
            if (collisionIt != collisionFlags.end() && collisionIt->second)
            {
                if (id >= 0)
                {
                    it->second.startFlash();
                }

                if (audioManager)
                {
                    audioManager->playCollisionSound(pos.x, pos.y, mainPos.x, mainPos.y);
                }
            }

            if (isNearby && audioManager)
            {
                if (audioManager->isEnginePlayingForCar(id))
                    audioManager->updateCarEngineVolume(id, pos.x, pos.y, mainPos.x, mainPos.y);
                else
                    audioManager->startCarEngine(id, pos.x, pos.y, mainPos.x, mainPos.y, false);
            }
        }
        else
        {
            // NEW CAR
            otherCars.emplace(id, Car(pos, typeId));

            auto collisionIt = collisionFlags.find(id);
            if (collisionIt != collisionFlags.end() && collisionIt->second)
            {
                if (id >= 0)
                {
                    otherCars[id].startFlash();
                }

                if (audioManager)
                {
                    audioManager->playCollisionSound(pos.x, pos.y, mainPos.x, mainPos.y);
                }
            }

            if (isNearby && audioManager)
                audioManager->startCarEngine(id, pos.x, pos.y, mainPos.x, mainPos.y, false);
        }

        previousCarPositions[id] = pos;
    }
}

void GameRenderer::cleanupRemovedCars(
    const std::map<int, std::pair<CarPosition, int>> &positions,
    const CarPosition &mainPos)
{
    for (auto it = otherCars.begin(); it != otherCars.end();)
    {
        int id = it->first;

        if (positions.find(id) == positions.end())
        {
            Car &car = it->second;

            if (!car.isExploding())
            {
                car.startExplosion();
                CarPosition pos = car.getPosition();
                if (audioManager)
                {
                    audioManager->playExplosionSound(pos.x, pos.y, mainPos.x, mainPos.y);
                    audioManager->stopCarEngine(id);
                }
                ++it;
            }
            else if (car.isExplosionComplete())
            {
                if (audioManager)
                    audioManager->stopCarEngine(id);

                previousCarPositions.erase(id);
                it = otherCars.erase(it);
            }
            else
            {
                ++it;
            }
        }
        else
        {
            ++it;
        }
    }
}

void GameRenderer::render(const CarPosition &mainCarPos, int mainCarTypeId, const std::map<int, std::pair<CarPosition, int>> &otherCarPositions,
                          const std::vector<Position> &next_checkpoints, bool mainCarCollisionFlag, const std::map<int, bool> &otherCarsCollisionFlags)
{
    updateMainCar(mainCarPos, mainCarCollisionFlag);
    setMainCarType(mainCarTypeId);
    updateOtherCars(otherCarPositions, otherCarsCollisionFlags);
    updateCheckpoints(next_checkpoints);

    camera.setScreenSize(logicalWidth, logicalHeight);
    camera.update(mainCar->getPosition());
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    renderBackground();
    
    renderCheckpoints();
    renderUpperLayer();

    std::vector<Car> on_bridge_cars;
    if (mainCar->getPosition().on_bridge)
    {
        on_bridge_cars.push_back(*mainCar);
    }
    else
    {
        renderCar(*mainCar);
    }

    for (auto &[id, car] : otherCars)
    {
        if (car.getPosition().on_bridge)
        {
            on_bridge_cars.push_back(car);
        }
        else
        {
            renderCar(car);
        }
    }

    renderUpperLayer();
    for (auto &car : on_bridge_cars)
    {
        renderCar(car);
    }
    minimap.render(renderer, *mainCar, otherCars, next_checkpoints, logicalWidth, logicalHeight);

    renderer.Present();
}

void GameRenderer::renderBackground()
{
    Vector2 camPos = camera.getPosition();
    renderer.Copy(
        backgroundTexture,
        NullOpt,
        Rect((int)-camPos.x, (int)-camPos.y,
             backgroundTexture.GetWidth(),
             backgroundTexture.GetHeight()));
}

void GameRenderer::renderCar(Car &car)
{
    Vector2 camPos = camera.getPosition();
    const CarPosition &pos = car.getPosition();
    SDL_Rect sprite = car.getSprite();
    int carScreenX = (int)(pos.x - camPos.x);
    int carScreenY = (int)(pos.y - camPos.y);

    if (car.isExploding())
    {
        car.updateExplosion();
        renderer.Copy(
            explosionSprites,
            Rect(sprite.x, sprite.y, sprite.w, sprite.h),
            Rect(carScreenX, carScreenY, sprite.w, sprite.h));
    }
    else
    {
        renderer.Copy(
            carSprites,
            Rect(sprite.x, sprite.y, sprite.w, sprite.h),
            Rect(carScreenX, carScreenY, sprite.w, sprite.h));

        if (car.isFlashing())
        {
            car.updateFlash();
            carSprites.SetBlendMode(SDL_BLENDMODE_ADD);
            Uint8 intensity = std::min(255, ((Car::FLASH_DURATION - car.getFlashFrame()) * 400) / Car::FLASH_DURATION);
            carSprites.SetAlphaMod(intensity);

            renderer.Copy(
                carSprites,
                Rect(sprite.x, sprite.y, sprite.w, sprite.h),
                Rect(carScreenX, carScreenY, sprite.w, sprite.h));
        }

        carSprites.SetBlendMode(SDL_BLENDMODE_BLEND);
        carSprites.SetAlphaMod(255);
    }
}

void GameRenderer::updateCheckpoints(const std::vector<Position> &positions)
{
    checkpoints.clear();
    for (size_t i = 0; i < positions.size(); ++i)
    {
        Checkpoint cp(positions[i], i, i < positions.size() - 1);
        if (i < positions.size() - 1)
        {
            cp.setNextCheckpoint(positions[i + 1]);
        }
        checkpoints.push_back(cp);
    }
}

void GameRenderer::renderCheckpoints()
{
    Vector2 camPos = camera.getPosition();
    for (const auto &checkpoint : checkpoints)
    {
        checkpoint.render(renderer, camPos);
    }
    if (!checkpoints.empty())
    {
        Checkpoint::renderScreenIndicator(
            renderer,
            checkpoints[0].getPosition(),
            camPos,
            logicalWidth,
            logicalHeight);
    }
}

void GameRenderer::renderUpperLayer()
{
    Vector2 camPos = camera.getPosition();
    for (const auto &rect : upperRects)
    {
        renderer.Copy(
            backgroundTexture,
            Rect((int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h),
            Rect((int)(rect.x - camPos.x), (int)(rect.y - camPos.y), (int)rect.w, (int)rect.h));
    }
}
