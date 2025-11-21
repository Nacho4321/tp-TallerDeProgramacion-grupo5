#include "client.h"
#include <iostream>
#include <string>
#include <SDL2/SDL.h>

Client::Client(const char *address, const char *port, StartMode mode, int join_game_id)
        : protocol(ini_protocol(address, port)),
            connected(true),
            handler(),
            handler_core(protocol),
            game_renderer("Game Renderer", 640, 480),
            start_mode(mode),
            auto_join_game_id(join_game_id)
{
    handler_core.start(); // iniciar handler (sender+receiver)
    
}

Client::~Client()
{
    handler_core.stop();
    handler_core.join();
}

void Client::start()
{
    // Ejecutar acción automática según el modo de inicio
    if (start_mode == StartMode::AUTO_CREATE) {
        std::cout << "[Client] AUTO_CREATE mode: Creating game automatically..." << std::endl;
        uint32_t gid = 0, pid = 0;
        bool ok = handler_core.create_game_blocking(gid, pid);
        if (ok) {
            std::cout << "[Client] Game created automatically. game_id=" << gid << " player_id=" << pid << std::endl;
            my_game_id = gid;
            my_player_id = static_cast<int32_t>(pid);
        } else {
            std::cout << "[Client] Failed to auto-create game!" << std::endl;
            connected = false;
            return;
        }
    } else if (start_mode == StartMode::AUTO_JOIN) {
        if (auto_join_game_id < 0) {
            std::cerr << "[Client] AUTO_JOIN mode requires valid game_id!" << std::endl;
            connected = false;
            return;
        }
        std::cout << "[Client] AUTO_JOIN mode: Joining game " << auto_join_game_id << " automatically..." << std::endl;
        uint32_t pid = 0;
        bool ok = handler_core.join_game_blocking(auto_join_game_id, pid);
        if (ok) {
            std::cout << "[Client] Joined game automatically. game_id=" << auto_join_game_id << " player_id=" << pid << std::endl;
            my_game_id = static_cast<uint32_t>(auto_join_game_id);
            my_player_id = static_cast<int32_t>(pid);
        } else {
            std::cout << "[Client] Failed to auto-join game " << auto_join_game_id << "!" << std::endl;
            connected = false;
            return;
        }
    }
    // Si llegamos aquí en modo AUTO_*, ya estamos en partida, continuar con el loop normal
    
    while (connected)
    {
        std::string input = handler.receive();
        if (input == "QUIT")
        {
            connected = false;
        }
        else if (!input.empty())
        {
            if (input == CREATE_GAME_STR) {
                std::cout << "[Client] Creating game..." << std::endl;
                uint32_t gid=0, pid=0;
                bool ok = handler_core.create_game_blocking(gid, pid);
                if (ok) {
                    std::cout << "[Client] Game created. game_id=" << gid << " player_id=" << pid << std::endl;
                    // Guardar mis IDs actuales
                    my_game_id = gid;
                    my_player_id = static_cast<int32_t>(pid);
                } else {
                    std::cout << "[Client] Failed to create game." << std::endl;
                }
            } else if (input.rfind(JOIN_GAME_STR, 0) == 0) {
                // formato: JOIN GAME <id>
                size_t last_space = input.find_last_of(' ');
                if (last_space != std::string::npos && last_space + 1 < input.size()) {
                    std::string game_id_str = input.substr(last_space + 1);
                    try {
                        int gid = std::stoi(game_id_str);
                        std::cout << "[Client] Joining game " << gid << "..." << std::endl;
                        uint32_t pid=0;
                        bool ok = handler_core.join_game_blocking(gid, pid);
                        if (ok) {
                            std::cout << "[Client] Joined game successfully. game_id=" << gid << " player_id=" << pid << std::endl;
                            // Guardar mis IDs actuales
                            my_game_id = static_cast<uint32_t>(gid);
                            my_player_id = static_cast<int32_t>(pid);
                        } else {
                            std::cout << "[Client] Failed to join game " << gid << ". ¿Existe esa partida? (Los IDs empiezan en 1)" << std::endl;
                        }
                    } catch (...) {
                        std::cerr << "[Client] Invalid game id in command: " << input << std::endl;
                    }
                } else {
                    std::cerr << "[Client] Invalid JOIN GAME command format. Use: JOIN GAME <id>" << std::endl;
                }
            } else {
                // comandos de movimiento u otros
                std::cout << "[Client] Sending: " << input << std::endl;
                handler_core.send(input);
            }
        }

        ServerMessage message;
        ServerMessage latest_message;
        bool got_message = false;

        while (handler_core.try_receive(message))
        {
            latest_message = message;
            got_message = true;
        }

        if (got_message && !latest_message.positions.empty())
        {
            // Elegir como "auto principal" el correspondiente a mi player_id (si lo tengo asignado)
            size_t idx_main = 0;
            if (my_player_id >= 0) {
                for (size_t i = 0; i < latest_message.positions.size(); ++i) {
                    if (latest_message.positions[i].player_id == my_player_id) {
                        idx_main = i;
                        break;
                    }
                }
            }

            const PlayerPositionUpdate& main_pos = latest_message.positions[idx_main];
            CarPosition mainCarPosition = CarPosition{
                main_pos.new_pos.new_X,
                main_pos.new_pos.new_Y,
                float(main_pos.new_pos.direction_x),
                float(main_pos.new_pos.direction_y)
            };

            std::vector<CarPosition> otherCars;
            for (size_t i = 0; i < latest_message.positions.size(); ++i)
            {
                if (i == idx_main) continue;
                const PlayerPositionUpdate& pos = latest_message.positions[i];
                otherCars.push_back(CarPosition{
                    pos.new_pos.new_X,
                    pos.new_pos.new_Y,
                    float(pos.new_pos.direction_x),
                    float(pos.new_pos.direction_y)
                });
            }

            const std::vector<Position> &next_cps = main_pos.next_checkpoints;
            game_renderer.render(mainCarPosition, otherCars, next_cps);
        }

        SDL_Delay(16);
    }
}