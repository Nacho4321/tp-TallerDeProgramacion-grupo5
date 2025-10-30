#include "input_handler.h"
#include "../common/protocol.h"  // To access the string constants

InputHandler::InputHandler() : prev_ticks(SDL_GetTicks()) {}

std::string InputHandler::receive() {
    prev_ticks = SDL_GetTicks();
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return "QUIT";
        } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    return "QUIT";
                case SDLK_LEFT:
                    return MOVE_LEFT_PRESSED_STR;
                case SDLK_RIGHT:
                    return MOVE_RIGHT_PRESSED_STR;
                case SDLK_UP:
                    return MOVE_UP_PRESSED_STR;
                case SDLK_DOWN:
                    return MOVE_DOWN_PRESSED_STR;
                default:
                    break;
            }
        } else if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    return MOVE_LEFT_RELEASED_STR;
                case SDLK_RIGHT:
                    return MOVE_RIGHT_RELEASED_STR;
                case SDLK_UP:
                    return MOVE_UP_RELEASED_STR;
                case SDLK_DOWN:
                    return MOVE_DOWN_RELEASED_STR;
                default:
                    break;
            }
        }
    }
    
    return ""; 
}