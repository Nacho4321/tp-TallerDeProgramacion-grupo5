#include <iostream>
#include <exception>
#include <SDL2pp/SDL2pp.hh>
#include "camera.h"
#include "renderer.h"

using namespace SDL2pp;

// Input handler class - handles keyboard input and updates position
class InputHandler {
private:
    CarPosition position;
    const float speed;
    unsigned int prev_ticks;

public:
    InputHandler(float initialX, float initialY, float moveSpeed = 400.0f) 
        : position{initialX, initialY, 0.0f, 0.0f}, 
          speed(moveSpeed), 
          prev_ticks(SDL_GetTicks()) {}

    // Process events and update position, returns false if should quit
    bool processInput() {
        // Calculate frame delta internally
        unsigned int frame_ticks = SDL_GetTicks();
        unsigned int frame_delta = frame_ticks - prev_ticks;
        prev_ticks = frame_ticks;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE || 
                    event.key.keysym.sym == SDLK_q) {
                    return false;
                }
            }
        }

        // Check keyboard state for arrow keys
        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        
        // Calculate movement based on frame time (speed is pixels per second)
        float movement = (speed * frame_delta) / 1000.0f;

        // Reset direction
        position.directionX = 0;
        position.directionY = 0;

        // Update position and direction based on input
        if (keystate[SDL_SCANCODE_LEFT]) {
            position.x -= movement;
            position.directionX = -1;
        }
        if (keystate[SDL_SCANCODE_RIGHT]) {
            position.x += movement;
            position.directionX = 1;
        }
        if (keystate[SDL_SCANCODE_UP]) {
            position.y -= movement;
            position.directionY = -1;
        }
        if (keystate[SDL_SCANCODE_DOWN]) {
            position.y += movement;
            position.directionY = 1;
        }

        return true;
    }

    const CarPosition& getPosition() const {
        return position;
    }

    void setPosition(float x, float y) {
        position.x = x;
        position.y = y;
    }

    // Keep car within bounds (world bounds, not screen bounds)
    void constrainToBounds(int width, int height, int carWidth, int carHeight) {
        if (position.x < 0) position.x = 0;
        if (position.y < 0) position.y = 0;
        if (position.x + carWidth > width) position.x = width - carWidth;
        if (position.y + carHeight > height) position.y = height - carHeight;
    }
};

// Main game class that coordinates everything
class Game {
private:
    SDL sdl;
    Window window;
    Renderer renderer;
    Texture sprites;
    Texture texture_backround;
    CarRenderer carRenderer;
    InputHandler inputHandler;

public:
    Game() 
        : sdl(SDL_INIT_VIDEO),
          window("SDL2pp Car Game with Camera",
                 SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                 640, 480,
                 SDL_WINDOW_RESIZABLE),
          renderer(window, -1, SDL_RENDERER_ACCELERATED),
          sprites(renderer, DATA_PATH "/cars/Mobile - Grand Theft Auto 4 - Miscellaneous - Cars.png"),
          texture_backround(renderer, DATA_PATH "/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Liberty City.png"),
          carRenderer(renderer, sprites, texture_backround),
          inputHandler(960, 540) {}  // Start at center of world

    void run() {
        while (true) {
            // Process input (returns false if should quit)
            if (!inputHandler.processInput()) {
                break;
            }

            // Keep car within world bounds
            inputHandler.constrainToBounds(
                texture_backround.GetWidth(), texture_backround.GetHeight(),  // World size
                30, 30       // Approximate car size
            );

            // Get current position from input handler
            const CarPosition& position = inputHandler.getPosition();

            // Render (camera logic is handled internally by CarRenderer)
            carRenderer.render(position);

            // Frame limiter
            SDL_Delay(1);
        }
    }
};

int main() try {
    Game game;
    game.run();
    return 0;
} catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
}