#include "input_handler.h"
#include "audio_manager.h"
#include "../common/protocol.h" // To access the string constants
#include <iostream>

InputHandler::InputHandler() : prev_ticks(SDL_GetTicks()) {}

std::string InputHandler::receive()
{
    prev_ticks = SDL_GetTicks();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        // Si estamos en modo de esperar el id del JOIN GAME
        if (awaiting_join_id) {
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                SDL_Keycode key = event.key.keysym.sym;

                // Enter: confirmar si hay algo en el buffer
                if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    if (!join_id_buffer.empty()) {
                        std::string cmd = std::string(JOIN_GAME_STR) + " " + join_id_buffer;
                        awaiting_join_id = false;
                        join_id_buffer.clear();
                        return cmd;
                    }
                    // si está vacío, ignorar enter
                    continue;
                }

                // Escape: cancelar modo join
                if (key == SDLK_ESCAPE) {
                    awaiting_join_id = false;
                    join_id_buffer.clear();
                    std::cout << "[Input] JOIN cancelado" << std::endl;
                    continue;
                }

                // Backspace: borrar último dígito
                if (key == SDLK_BACKSPACE) {
                    if (!join_id_buffer.empty()) join_id_buffer.pop_back();
                    continue;
                }

                // Aceptar dígitos 0-9 (incluye keypad)
                if ((key >= SDLK_0 && key <= SDLK_9)) {
                    join_id_buffer.push_back(char('0' + (key - SDLK_0)));
                    continue;
                }
                if (key >= SDLK_KP_0 && key <= SDLK_KP_9) {
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
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
            case SDLK_q:
                return "QUIT";
            case SDLK_w:
                if (audioManager) {
                    audioManager->increaseMasterVolume();
                }
                return "";  // Don't send to server
            case SDLK_s:
                if (audioManager) {
                    audioManager->decreaseMasterVolume();
                }
                return "";  // Don't send to server
            case SDLK_j:
                // Activar modo para ingresar id de JOIN GAME
                awaiting_join_id = true;
                join_id_buffer.clear();
                std::cout << "[Input] Ingresá el id de partida y presioná Enter..." << std::endl;
                return ""; // No enviar nada aún
            case SDLK_c:
                return CREATE_GAME_STR;
            case SDLK_i:
                return START_GAME_STR;
            case SDLK_LEFT:
                return MOVE_LEFT_PRESSED_STR;
            case SDLK_RIGHT:
                return MOVE_RIGHT_PRESSED_STR;
            case SDLK_UP:
                return MOVE_UP_PRESSED_STR;
            case SDLK_DOWN:
                return MOVE_DOWN_PRESSED_STR;
            case SDLK_1:
                return std::string(CHANGE_CAR_STR) + " lambo";
            case SDLK_2:
                return std::string(CHANGE_CAR_STR) + " truck";
            case SDLK_3:
                return std::string(CHANGE_CAR_STR) + " sports_car";
            case SDLK_4:
                return std::string(CHANGE_CAR_STR) + " rally";
            case SDLK_5:
                return std::string(CHANGE_CAR_STR) + " lambo"; // futuro tipo
            case SDLK_6:
                return std::string(CHANGE_CAR_STR) + " truck"; // futuro tipo
            default:
                break;
            }
        }
        else if (event.type == SDL_KEYUP)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_LEFT:
                return MOVE_LEFT_RELEASED_STR;
            case SDLK_RIGHT:
                return MOVE_RIGHT_RELEASED_STR;
            case SDLK_UP:
                return MOVE_UP_RELEASED_STR;
            case SDLK_DOWN:
                return MOVE_DOWN_RELEASED_STR;
            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
                // Ignorar keyup para cambio de auto (solo en keydown)
                break;
            default:
                break;
            }
        }
    }

    return "";
}