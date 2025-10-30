// input_handler.h
#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <string>
#include <SDL2/SDL.h>
#include <unordered_map>

class InputHandler {
private:
    unsigned int prev_ticks;
    std::unordered_map<SDL_Scancode, bool> key_states; 
    
public:
    InputHandler();
    std::string receive();
};

#endif