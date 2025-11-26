// input_handler.h
#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <string>
#include <SDL2/SDL.h>
#include <unordered_map>
#include <functional>

class InputHandler {
private:
    unsigned int prev_ticks;
    std::unordered_map<SDL_Scancode, bool> key_states; 
    
    // Mapas para dispatch de teclas a comandos
    std::unordered_map<SDL_Keycode, std::string> keydown_actions;
    std::unordered_map<SDL_Keycode, std::string> keyup_actions;
    std::unordered_map<SDL_Keycode, std::function<std::string()>> keydown_special; // Para casos especiales
    
    // Modo de ingreso para JOIN GAME: esperar un id numérico
    bool awaiting_join_id = false;
    std::string join_id_buffer;
    
    // Inicializar mapas de dispatch
    void init_key_maps();
    
public:
    InputHandler();
    std::string receive();
};

#endif