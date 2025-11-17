#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H
#include <SDL2pp/SDL2pp.hh>
#include <vector>
#include <memory>
#include "camera.h"
#include "car.h"
#include "minimap.h"  
using namespace SDL2pp;

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

public:
    GameRenderer(const char* windowTitle, int windowWidth, int windowHeight)
        : sdl(SDL_INIT_VIDEO),
          window(windowTitle,
                 SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                 windowWidth, windowHeight,
                 SDL_WINDOW_RESIZABLE),
          renderer(window, -1, SDL_RENDERER_ACCELERATED),
          carSprites(renderer, [] {
              SDL2pp::Surface surface("data/cars/Mobile - Grand Theft Auto 4 - Miscellaneous - Cars.png");
              surface.SetColorKey(true, SDL_MapRGB(surface.Get()->format, 0xA3, 0xA3, 0x0D));
              return surface;
          }()),
          backgroundTexture(renderer, Surface("data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Liberty City.png")),
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

    void render() {
        camera.setScreenSize(renderer.GetOutputWidth(), renderer.GetOutputHeight());
        camera.update(mainCar->getPosition());
        renderer.Clear();
        renderBackground();
        renderCar(*mainCar);
        for (const auto& car : otherCars) {
            renderCar(car);
        }
        
        minimap.render(renderer, *mainCar, otherCars);
        renderer.Present();
    }

    void render(const CarPosition& mainCarPos, const std::vector<CarPosition>& otherCarPositions) {
        updateMainCar(mainCarPos);
        updateOtherCars(otherCarPositions);
        render();
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
};
#endif // GAME_RENDERER_H