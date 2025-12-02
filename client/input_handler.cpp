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
    keydown_actions[SDLK_c] = CREATE_GAME_STR;
    keydown_actions[SDLK_i] = START_GAME_STR;

    // Teclas de mejora de auto (upgrade)
    keydown_special[SDLK_1] = []()
    { return std::string(UPGRADE_CAR_STR) + " " + std::to_string(static_cast<int>(CarUpgrade::ACCELERATION_BOOST)); };
    keydown_special[SDLK_2] = []()
    { return std::string(UPGRADE_CAR_STR) + " " + std::to_string(static_cast<int>(CarUpgrade::SPEED_BOOST)); };
    keydown_special[SDLK_3] = []()
    { return std::string(UPGRADE_CAR_STR) + " " + std::to_string(static_cast<int>(CarUpgrade::HANDLING_IMPROVEMENT)); };
    keydown_special[SDLK_4] = []()
    { return std::string(UPGRADE_CAR_STR) + " " + std::to_string(static_cast<int>(CarUpgrade::DURABILITY_ENHANCEMENT)); };

    // Teclas de cheats: P O L K M
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
        // Si estamos en modo de esperar el id del JOIN GAME
        if (awaiting_join_id)
        {
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
            {
                SDL_Keycode key = event.key.keysym.sym;

                // Enter: confirmar si hay algo en el buffer
                if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
                {
                    if (!join_id_buffer.empty())
                    {
                        std::string cmd = std::string(JOIN_GAME_STR) + " " + join_id_buffer;
                        awaiting_join_id = false;
                        join_id_buffer.clear();
                        return cmd;
                    }
                    // si está vacío, ignorar enter
                    continue;
                }

                // Escape: cancelar modo join
                if (key == SDLK_ESCAPE)
                {
                    awaiting_join_id = false;
                    join_id_buffer.clear();
                    std::cout << "[Input] JOIN cancelado" << std::endl;
                    continue;
                }

                // Backspace: borrar último dígito
                if (key == SDLK_BACKSPACE)
                {
                    if (!join_id_buffer.empty())
                        join_id_buffer.pop_back();
                    continue;
                }

                // Aceptar dígitos 0-9 (incluye keypad)
                if ((key >= SDLK_0 && key <= SDLK_9))
                {
                    join_id_buffer.push_back(char('0' + (key - SDLK_0)));
                    continue;
                }
                if (key >= SDLK_KP_0 && key <= SDLK_KP_9)
                {
                    join_id_buffer.push_back(char('0' + (key - SDLK_KP_0)));
                    continue;
                }

                // Ignorar otras teclas mientras se ingresa el id
                continue;
            }
            // Mientras estamos esperando id, ignorar otros eventos
            continue;
        }

        if (event.type == SDL_QUIT)
        {
            return "QUIT";
        }
        else if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
        {
            SDL_Keycode key = event.key.keysym.sym;

            // Casos especiales: QUIT y JOIN
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
                return ""; // Don't send to server
            }
            if (key == SDLK_s)
            {
                if (audioManager)
                {
                    audioManager->decreaseMasterVolume();
                }
                return ""; // Don't send to server
            }
            if (key == SDLK_j)
            {
                // Activar modo para ingresar id de JOIN GAME
                awaiting_join_id = true;
                join_id_buffer.clear();
                std::cout << "[Input] Ingresá el id de partida y presioná Enter..." << std::endl;
                return ""; // No enviar nada aún
            }

            // Buscar en acciones especiales (lambdas)
            auto special_it = keydown_special.find(key);
            if (special_it != keydown_special.end())
            {
                return special_it->second(); // Invocar lambda
            }

            // Buscar en acciones simples
            auto action_it = keydown_actions.find(key);
            if (action_it != keydown_actions.end())
            {
                return action_it->second;
            }
        }
        else if (event.type == SDL_KEYUP)
        {
            SDL_Keycode key = event.key.keysym.sym;

            // Buscar en acciones de keyup
            auto it = keyup_actions.find(key);
            if (it != keyup_actions.end())
            {
                return it->second;
            }

            // Ignorar keyup de teclas de cambio de auto (solo actúan en keydown)
            if (keydown_special.find(key) != keydown_special.end())
            {
                // Es una tecla de cambio de auto, ignorar el keyup
                return "";
            }
        }
    }

    return "";
}