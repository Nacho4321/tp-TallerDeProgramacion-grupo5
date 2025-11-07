#include "client.h"
#include <iostream>
#include <string>
#include <SDL2/SDL.h>

Client::Client(const char *address, const char *port)
    : protocol(ini_protocol(address, port)),
      connected(true),
      handler(),
      handler_core(protocol),
      car_renderer("Game Renderer", 640, 480)
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
            PlayerPositionUpdate pos_updated = latest_message.positions.front();

            CarPosition position = CarPosition{
                pos_updated.new_pos.new_X,
                pos_updated.new_pos.new_Y,
                float(pos_updated.new_pos.direction_x),
                float(pos_updated.new_pos.direction_y)};

            car_renderer.render(position);
        }

        SDL_Delay(16);
    }
}