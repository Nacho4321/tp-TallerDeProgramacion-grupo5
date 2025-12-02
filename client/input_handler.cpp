#include "input_handler.h"
#include "audio_manager.h"
#include "../common/protocol.h" // To access the string constants
#include <iostream>

InputHandler::InputHandler() : prev_ticks(SDL_GetTicks())
{
    init_key_maps();
}

void InputHandler::init_key_maps()
{
    // Mapeo de teclas presionadas (KEYPRESSED)
    keydown_actions[SDLK_LEFT] = MOVE_LEFT_PRESSED_STR;
    keydown_actions[SDLK_RIGHT] = MOVE_RIGHT_PRESSED_STR;
    keydown_actions[SDLK_UP] = MOVE_UP_PRESSED_STR;
    keydown_actions[SDLK_DOWN] = MOVE_DOWN_PRESSED_STR;

    // Teclas de mejora de auto (upgrade)
    keydown_special[SDLK_1] = []()
    { return std::string(UPGRADE_CAR_STR) + " " + std::to_string(static_cast<int>(CarUpgrade::ACCELERATION_BOOST)); };
    keydown_special[SDLK_2] = []()
    { return std::string(UPGRADE_CAR_STR) + " " + std::to_string(static_cast<int>(CarUpgrade::SPEED_BOOST)); };
    keydown_special[SDLK_3] = []()
    { return std::string(UPGRADE_CAR_STR) + " " + std::to_string(static_cast<int>(CarUpgrade::HANDLING_IMPROVEMENT)); };
    keydown_special[SDLK_4] = []()
    { return std::string(UPGRADE_CAR_STR) + " " + std::to_string(static_cast<int>(CarUpgrade::DURABILITY_ENHANCEMENT)); };

    // Teclas de cheats: P O L K
    keydown_actions[SDLK_p] = CHEAT_GOD_MODE_STR;     // P = God mode (vida infinita toggle)
    keydown_actions[SDLK_o] = CHEAT_SKIP_LAP_STR;     // O = Completar ronda actual
    keydown_actions[SDLK_l] = CHEAT_DIE_STR;          // L = Morir/perder automáticamente
    keydown_actions[SDLK_k] = CHEAT_FULL_UPGRADE_STR; // K = Mejoras al máximo

    // Mapeo de teclas soltadas (KEYRELEASED)
    keyup_actions[SDLK_LEFT] = MOVE_LEFT_RELEASED_STR;
    keyup_actions[SDLK_RIGHT] = MOVE_RIGHT_RELEASED_STR;
    keyup_actions[SDLK_UP] = MOVE_UP_RELEASED_STR;
    keyup_actions[SDLK_DOWN] = MOVE_DOWN_RELEASED_STR;
}

std::string InputHandler::receive()
{
    prev_ticks = SDL_GetTicks();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            return "QUIT";
        }
        else if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
        {
            SDL_Keycode key = event.key.keysym.sym;

            if (key == SDLK_ESCAPE || key == SDLK_q)
            {
                return "QUIT";
            }
            if (key == SDLK_w)
            {
                if (audioManager)
                {
                    audioManager->increaseMasterVolume();
                }
                return ""; // No va al server
            }
            if (key == SDLK_s)
            {
                if (audioManager)
                {
                    audioManager->decreaseMasterVolume();
                }
                return "";
            }

            auto special_it = keydown_special.find(key);
            if (special_it != keydown_special.end())
            {
                return special_it->second();
            }

            auto action_it = keydown_actions.find(key);
            if (action_it != keydown_actions.end())
            {
                return action_it->second;
            }
        }
        else if (event.type == SDL_KEYUP)
        {
            SDL_Keycode key = event.key.keysym.sym;

            auto it = keyup_actions.find(key);
            if (it != keyup_actions.end())
            {
                return it->second;
            }

            if (keydown_special.find(key) != keydown_special.end())
            {
                // Es una tecla de cambio de auto, ignorar el keyup
                return "";
            }
        }
    }

    return "";
}