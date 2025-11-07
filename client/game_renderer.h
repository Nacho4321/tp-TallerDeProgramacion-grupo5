#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <SDL2pp/SDL2pp.hh>
#include <vector>
#include <memory>
#include "camera.h"
#include "car.h"

using namespace SDL2pp;

class GameRenderer {
private:
    SDL sdl;
    Window window;
    Renderer renderer;
    Texture carSprites;
    Texture backgroundTexture;
    Camera camera;
    
    // Main car (camera follows this one)
    std::unique_ptr<Car> mainCar;
    
    // Other cars to render
    std::vector<Car> otherCars;
    
public:
    GameRenderer(const char* windowTitle, int windowWidth, int windowHeight)
        : sdl(SDL_INIT_VIDEO),
        window(windowTitle,
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                windowWidth, windowHeight,
                SDL_WINDOW_RESIZABLE),
        renderer(window, -1, SDL_RENDERER_ACCELERATED),

        // Create a surface, set color key, then build the texture
        carSprites(renderer, [] {
            SDL2pp::Surface surface("data/cars/Mobile - Grand Theft Auto 4 - Miscellaneous - Cars.png");
            surface.SetColorKey(true, SDL_MapRGB(surface.Get()->format, 0xA3, 0xA3, 0x0D));
            return surface;
        }()),

        backgroundTexture(renderer, Surface("data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Liberty City.png")),
        camera(windowWidth, windowHeight),
        mainCar(std::make_unique<Car>()) {}

    
    // Update main car position
    void updateMainCar(const CarPosition& position) {
        mainCar->setPosition(position);
    }
    
    // Update other cars positions
    void updateOtherCars(const std::vector<CarPosition>& positions) {
        otherCars.clear();
        
        for (const auto& pos : positions) {
            Car newCar(pos);           // Create a Car object
            otherCars.push_back(newCar); // Add it to the vector
        }
    }
    
    // Render everything
    void render() {
        // Update camera to follow main car
        camera.setScreenSize(renderer.GetOutputWidth(), renderer.GetOutputHeight());
        camera.update(mainCar->getPosition());
        
        // Clear renderer
        renderer.Clear();
        
        // Render background
        renderBackground();
        
        // Render all cars
        renderCar(*mainCar);
        
        for (const auto& car : otherCars) {
            renderCar(car);
        }
        
        // Present
        renderer.Present();
    }
    
    // Alternative render method that takes positions directly
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