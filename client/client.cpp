#include "client.h"
#include "../common/constants.h"
#include <iostream>
#include <string>
#include <cmath>
#include <SDL2/SDL.h>
#include <map>

Client::Client(const char *address, const char *port, StartMode mode, int join_game_id, const std::string &game_name)
    : protocol(ini_protocol(address, port)),
      connected(true),
      handler(),
      handler_core(protocol),
      game_renderer("Game Renderer", LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 1, "data/cities/liberty_city.json"),
      start_mode(mode),
      auto_join_game_id(join_game_id),
      auto_create_game_name(game_name)
{
    handler_core.start(); // iniciar handler (sender+receiver)
    handler.setAudioManager(game_renderer.getAudioManager());
}

Client::~Client()
{
    handler_core.stop();
    handler_core.join();
}

void Client::start()
{
    // Ejecutar acción automática según el modo de inicio
    if (start_mode == StartMode::AUTO_CREATE)
    {
        uint32_t gid = 0, pid = 0;
        bool ok = handler_core.create_game_blocking(gid, pid, auto_create_game_name);
        if (ok)
        {
            my_game_id = gid;
            my_player_id = static_cast<int32_t>(pid);
        }
        else
        {
            std::cout << "[Client] Failed to autocreate game" << std::endl;
            connected = false;
            return;
        }
    }
    else if (start_mode == StartMode::AUTO_JOIN)
    {
        if (auto_join_game_id < 0)
        {
            std::cerr << "[Client] AUTOJOIN mode requires validid!" << std::endl;
            connected = false;
            return;
        }
        std::cout << "[Client] AUTOJOIN mode: Joining game " << auto_join_game_id << std::endl;
        uint32_t pid = 0;
        bool ok = handler_core.join_game_blocking(auto_join_game_id, pid);
        if (ok)
        {
            std::cout << "[Client] Joined game. game_id=" << auto_join_game_id << " player_id=" << pid << std::endl;
            my_game_id = static_cast<uint32_t>(auto_join_game_id);
            my_player_id = static_cast<int32_t>(pid);
        }
        else
        {
            std::cout << "[Client] Failed to auto-join game " << auto_join_game_id << std::endl;
            connected = false;
            return;
        }
    }

    while (connected)
    {
        std::string input = handler.receive();
        if (input == "QUIT")
        {
            connected = false;
        }
        else if (!input.empty())
        {
            if (input == CREATE_GAME_STR)
            {
                std::cout << "[Client] Creating game..." << std::endl;
                uint32_t gid = 0, pid = 0;
                bool ok = handler_core.create_game_blocking(gid, pid);
                if (ok)
                {
                    std::cout << "[Client] Game created. game_id=" << gid << " player_id=" << pid << std::endl;
                    // Guardar mis IDs actuales
                    my_game_id = gid;
                    my_player_id = static_cast<int32_t>(pid);
                }
                else
                {
                    std::cout << "[Client] Failed to create game." << std::endl;
                }
            }
            else if (input.rfind(JOIN_GAME_STR, 0) == 0)
            {
                // formato: JOIN GAME <id>
                size_t last_space = input.find_last_of(' ');
                if (last_space != std::string::npos && last_space + 1 < input.size())
                {
                    std::string game_id_str = input.substr(last_space + 1);
                    try
                    {
                        int gid = std::stoi(game_id_str);
                        std::cout << "[Client] Joining game " << gid << "..." << std::endl;
                        uint32_t pid = 0;
                        bool ok = handler_core.join_game_blocking(gid, pid);
                        if (ok)
                        {
                            std::cout << "[Client] Joined game successfully. game_id=" << gid << " player_id=" << pid << std::endl;
                            // Guardar mis IDs actuales
                            my_game_id = static_cast<uint32_t>(gid);
                            my_player_id = static_cast<int32_t>(pid);
                        }
                        else
                        {
                            std::cout << "[Client] Failed to join game " << gid << ". ¿Existe esa partida? (Los IDs empiezan en 1)" << std::endl;
                        }
                    }
                    catch (...)
                    {
                        std::cerr << "[Client] Invalid game id in command: " << input << std::endl;
                    }
                }
                else
                {
                    std::cerr << "[Client] Invalid JOIN GAME command format. Use: JOIN GAME <id>" << std::endl;
                }
            }
            else
            {
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
            bool player_found = false;

            if (my_player_id >= 0)
            {
                for (size_t i = 0; i < latest_message.positions.size(); ++i)
                {
                    if (latest_message.positions[i].player_id == my_player_id)
                    {
                        idx_main = i;
                        player_found = true;
                        break;
                    }
                }
            }
            else
            {
                player_found = true;
            }

            if (!player_found && my_player_id >= 0)
            {
                if (game_renderer.mainCar && !game_renderer.mainCar->isExploding())
                {
                    game_renderer.mainCar->startExplosion();

                    CarPosition deathPos = game_renderer.mainCar->getPosition();
                    game_renderer.getAudioManager()->playExplosionSound(
                        deathPos.x, deathPos.y, deathPos.x, deathPos.y);
                    game_renderer.getAudioManager()->stopCarEngine(-1);
                }

                if (game_renderer.mainCar && game_renderer.mainCar->isExplosionComplete())
                {
                    idx_main = 0;
                    player_found = true;
                    game_renderer.mainCar->stopExplosion();
                    my_player_id = SPECTATOR_MODE;  // Switch to spectator mode
                }
            }

            CarPosition mainCarPosition;
            int mainTypeId = 0;
            std::vector<Position> next_cps;
            bool mainCarCollisionFlag = false;

            if (player_found)
            {
                const PlayerPositionUpdate &main_pos = latest_message.positions[idx_main];
                double angle = main_pos.new_pos.angle;
                mainCarPosition = CarPosition{
                    main_pos.new_pos.new_X,
                    main_pos.new_pos.new_Y,
                    float(-std::sin(angle)),
                    float(std::cos(angle)),
                    main_pos.new_pos.on_bridge};
                auto it = car_type_map.find(main_pos.car_type);
                mainTypeId = (it != car_type_map.end()) ? it->second : 0;
                next_cps = main_pos.next_checkpoints;
                mainCarCollisionFlag = main_pos.collision_flag;
            }
            else if (game_renderer.mainCar)
            {
                mainCarPosition = game_renderer.mainCar->getPosition();
            }

            std::map<int, std::pair<CarPosition, int>> otherCars;
            std::map<int, bool> otherCarsCollisionFlags;
            for (size_t i = 0; i < latest_message.positions.size(); ++i)
            {
                if (i == idx_main)
                    continue;
                const PlayerPositionUpdate &pos = latest_message.positions[i];
                double ang = pos.new_pos.angle;
                CarPosition cp{
                    pos.new_pos.new_X,
                    pos.new_pos.new_Y,
                    float(-std::sin(ang)),
                    float(std::cos(ang)),
                    pos.new_pos.on_bridge};
                auto other_it = car_type_map.find(pos.car_type);
                int other_type_id = (other_it != car_type_map.end()) ? other_it->second : 0;
                otherCars[pos.player_id] = std::make_pair(cp, other_type_id);
                otherCarsCollisionFlags[pos.player_id] = pos.collision_flag;
            }

            game_renderer.render(mainCarPosition, mainTypeId, otherCars, next_cps, mainCarCollisionFlag, otherCarsCollisionFlags);
        }

        SDL_Delay(16);
    }
}