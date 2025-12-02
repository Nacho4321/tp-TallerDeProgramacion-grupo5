#include "client.h"
#include "../common/constants.h"
#include <iostream>
#include <string>
#include <cmath>
#include <SDL2/SDL.h>
#include <map>

// helper para crear conexion como antes
void Client::initLegacyConnection(const char* address, const char* port) {
    Socket sock(address, port);
    owned_protocol_ = std::make_unique<Protocol>(std::move(sock));
    owned_handler_ = std::make_unique<GameClientHandler>(*owned_protocol_);
    active_handler_ = owned_handler_.get();
    owned_handler_->start();
}

Client::Client(const char *address, const char *port, StartMode mode, int join_game_id, const std::string &game_name)
    : connection_(nullptr),
      owned_protocol_(nullptr),
      owned_handler_(nullptr),
      active_handler_(nullptr),
      connected(true),
      handler(),
      game_renderer("Game Renderer", LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, LIBERTY_CITY_MAP_ID, "data/cities/liberty_city.json"),
      start_mode(mode),
      auto_join_game_id(join_game_id),
      auto_create_game_name(game_name)
{
    initLegacyConnection(address, port);
    handler.setAudioManager(game_renderer.getAudioManager());
}

// nuevo constructor desde lobby (QT)
Client::Client(std::unique_ptr<GameConnection> connection)
    : connection_(std::move(connection)),
      owned_protocol_(nullptr),
      owned_handler_(nullptr),
      active_handler_(nullptr),
      connected(true),
      handler(),
      game_renderer("Game Renderer", LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 1, "data/cities/liberty_city.json"),
      start_mode(StartMode::FROM_LOBBY),
      auto_join_game_id(-1),
      auto_create_game_name("")
{
    active_handler_ = connection_->getHandler();
    
    my_game_id = connection_->getGameId();
    my_player_id = static_cast<int32_t>(connection_->getPlayerId());
    original_player_id = my_player_id;
    
    handler.setAudioManager(game_renderer.getAudioManager());
}

Client::~Client()
{
    if (connection_) {
        connection_->stop();
        connection_->join();
    }
    else if (owned_handler_) {
        owned_handler_->stop();
        owned_handler_->join();
    }
}

void Client::start()
{
    // Ejecutar acción automática según el modo de inicio
    if (start_mode == StartMode::AUTO_CREATE) {
        uint32_t gid = 0, pid = 0;
        bool ok = active_handler_->create_game_blocking(gid, pid, auto_create_game_name);
        if (ok)
        {
            my_game_id = gid;
            my_player_id = static_cast<int32_t>(pid);
            original_player_id = my_player_id;
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
        bool ok = active_handler_->join_game_blocking(auto_join_game_id, pid);
        if (ok)
        {
            std::cout << "[Client] Joined game. game_id=" << auto_join_game_id << " player_id=" << pid << std::endl;
            my_game_id = static_cast<uint32_t>(auto_join_game_id);
            my_player_id = static_cast<int32_t>(pid);
            original_player_id = my_player_id;
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
                bool ok = active_handler_->create_game_blocking(gid, pid);
                if (ok)
                {
                    std::cout << "[Client] Game created. game_id=" << gid << " player_id=" << pid << std::endl;
                    // Guardar mis IDs actuales
                    my_game_id = gid;
                    my_player_id = static_cast<int32_t>(pid);
                    original_player_id = my_player_id;
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
                        bool ok = active_handler_->join_game_blocking(gid, pid);
                        if (ok)
                        {
                            std::cout << "[Client] Joined game successfully. game_id=" << gid << " player_id=" << pid << std::endl;
                            // Guardar mis IDs actuales
                            my_game_id = static_cast<uint32_t>(gid);
                            my_player_id = static_cast<int32_t>(pid);
                            original_player_id = my_player_id;
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
                active_handler_->send(input);
            }
        }

    ServerMessage message;
    ServerMessage latest_message;
    bool got_message = false;
    uint8_t latest_opcode = 0;
    bool saw_starting_countdown = false;
    bool saw_race_times = false;
    bool saw_total_times = false;
    ServerMessage last_race_times_msg;
    ServerMessage last_total_times_msg;

        while (active_handler_->try_receive(message))
        {
            // Track the last message for rendering, but also note if any STARTING_COUNTDOWN arrived
            if (message.opcode == STARTING_COUNTDOWN)
            {
                saw_starting_countdown = true;
            }
            else if (message.opcode == RACE_TIMES)
            {
                saw_race_times = true;
                last_race_times_msg = message;
            }
            else if (message.opcode == TOTAL_TIMES)
            {
                saw_total_times = true;
                last_total_times_msg = message;
            }

            latest_message = message;
            got_message = true;
            latest_opcode = latest_message.opcode;
        }

        // Aviso simple de inicio de countdown
        if (got_message && (latest_opcode == STARTING_COUNTDOWN || saw_starting_countdown))
        {
            std::cout << "[Client] STARTING countdown begun (10s)." << std::endl;
            game_renderer.startCountDown();
        }

        if (saw_race_times || saw_total_times)
        {
            std::cout << my_player_id << original_player_id << std::endl;
            my_player_id = original_player_id;
            
            game_renderer.winSound();
            game_renderer.showResults(
                saw_race_times ? last_race_times_msg.race_times : std::vector<ServerMessage::PlayerRaceTime>(),
                saw_total_times ? last_total_times_msg.total_times : std::vector<ServerMessage::PlayerTotalTime>(),
                original_player_id  
            );
        }

        if (got_message && latest_message.opcode == UPDATE_POSITIONS && !latest_message.positions.empty())
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
            bool mainCarIsStopping = false;
            float mainCarHP = 100.0f;

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
                mainCarIsStopping = main_pos.is_stopping;
                mainCarHP = main_pos.hp;
            }
            else if (game_renderer.mainCar)
            {
                mainCarPosition = game_renderer.mainCar->getPosition();
            }

            std::map<int, std::pair<CarPosition, int>> otherCars;
            std::map<int, bool> otherCarsCollisionFlags;

            for (size_t i = 0; i < latest_message.positions.size(); ++i)
            {
                const PlayerPositionUpdate &pos = latest_message.positions[i];

                if (pos.player_id == original_player_id)
                    continue;

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

            game_renderer.render(mainCarPosition, mainTypeId, otherCars, next_cps, mainCarCollisionFlag, mainCarIsStopping, mainCarHP, otherCarsCollisionFlags);
        }

        SDL_Delay(16);
    }
}