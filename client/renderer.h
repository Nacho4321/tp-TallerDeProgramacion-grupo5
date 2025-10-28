#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2pp/SDL2pp.hh>
#include <SDL2/SDL.h>
#include <cmath>
#include "camera.h"

using namespace SDL2pp;

// Array of car sprite rectangles
static SDL_Rect cars[] = {
    { 2, 5, 28, 22}, { 32, 3, 31, 25}, { 65, 2, 29, 29},
    { 99, 1, 25, 30}, { 133, 2, 22, 28}, { 164, 1, 25, 30},
    { 194, 2, 29, 29}, { 225, 3, 31, 25}, { 2, 37, 28, 22},
    { 32, 36, 32, 26}, { 66, 33, 29, 29}, { 99, 33, 26, 30},
    { 133, 33, 22, 29}, { 163, 33, 25, 30}, { 193, 33, 29, 29},
    { 224, 36, 31, 25}
};

class CarRenderer {
private:
    SDL sdl;
    Window window;
    Renderer renderer;
    Texture sprites;
    Texture texture_backround;
    Camera camera;  
    const int num_sprites;
    unsigned int prev_ticks;
    int sprite_index;

public:
    CarRenderer(const char* windowTitle, 
                int windowWidth, int windowHeight,
                const char* spritesPath,
                const char* backgroundPath) 
        : sdl(SDL_INIT_VIDEO),
          window(windowTitle,
                 SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                 windowWidth, windowHeight,
                 SDL_WINDOW_RESIZABLE),
          renderer(window, -1, SDL_RENDERER_ACCELERATED),
          sprites(renderer, "data/cars/Mobile - Grand Theft Auto 4 - Miscellaneous - Cars.png"),
          texture_backround(renderer, "data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Liberty City.png"),
          camera(windowWidth, windowHeight),
          num_sprites(sizeof(cars) / sizeof(SDL_Rect)),
          prev_ticks(SDL_GetTicks()),
          sprite_index(0) {}

    void render(const CarPosition& position) {
        unsigned int frame_ticks = SDL_GetTicks();
        unsigned int frame_delta = frame_ticks - prev_ticks;
        prev_ticks = frame_ticks;

        camera.setScreenSize(renderer.GetOutputWidth(), renderer.GetOutputHeight());
        
        camera.update(position);

        if (position.directionX != 0 || position.directionY != 0) {
            double angle = atan2(position.directionY, position.directionX);
            
            double degrees = angle * 180.0 / M_PI;
            if (degrees < 0) degrees += 360;

            degrees = fmod(degrees, 360);

            sprite_index = (int)round(degrees / 22.5) % num_sprites;
        }

        renderer.Clear();

        Vector2 camPos = camera.getPosition();
        
        renderer.Copy(
            texture_backround,
            NullOpt,
            Rect((int)-camPos.x, (int)-camPos.y, 
                 texture_backround.GetWidth(), texture_backround.GetHeight())
        );

        int carScreenX = (int)(position.x - camPos.x);
        int carScreenY = (int)(position.y - camPos.y);
        
        renderer.Copy(
            sprites,
            Rect(cars[sprite_index].x, cars[sprite_index].y, 
                 cars[sprite_index].w, cars[sprite_index].h),
            Rect(carScreenX, carScreenY, 
                 cars[sprite_index].w, cars[sprite_index].h)
        );

        renderer.Present();
    }

    int getBackgroundWidth() const {
        return texture_backround.GetWidth();
    }

    int getBackgroundHeight() const {
        return texture_backround.GetHeight();
    }
};

#endif // RENDERER_H