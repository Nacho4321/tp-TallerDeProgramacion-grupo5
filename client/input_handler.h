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
    // Modo de ingreso para JOIN GAME: esperar un id numérico
    bool awaiting_join_id = false;
    std::string join_id_buffer;
    
public:
    InputHandler();
    std::string receive();
};

#endif