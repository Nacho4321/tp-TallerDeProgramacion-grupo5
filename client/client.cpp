#include "client.h"
#include "game_state_tracker.h"
#include "player_state_tracker.h"
#include "position_update_handler.h"
#include "../common/constants.h"
#include "install_paths.h"
#include <iostream>
#include <string>
#include <cmath>
#include <SDL2/SDL.h>
#include <map>

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
      game_renderer("Game Renderer", LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, static_cast<int>(MapId::LibertyCity), std::string(DATA_DIR) + "/cities/liberty_city.json"),
      start_mode(mode),
      auto_join_game_id(join_game_id),
      auto_create_game_name(game_name)
{
    initLegacyConnection(address, port);
    handler.setAudioManager(game_renderer.getAudioManager());
}

Client::Client(std::unique_ptr<GameConnection> connection)
    : connection_(std::move(connection)),
      owned_protocol_(nullptr),
      owned_handler_(nullptr),
      active_handler_(nullptr),
      connected(true),
      handler(),
      game_renderer("Game Renderer", LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT,
                    connection_ ? connection_->getMapId() : 0,
                    connection_ ? MAP_JSON_PATHS[connection_->getMapId()] : (std::string(DATA_DIR) + "/cities/liberty_city.json")),
      start_mode(StartMode::FROM_LOBBY),
      auto_join_game_id(-1),
      auto_create_game_name("")
{
    active_handler_ = connection_->getHandler();

    my_game_id = connection_->getGameId();
    int32_t player_id = static_cast<int32_t>(connection_->getPlayerId());
    playerTracker.setPlayerId(player_id);
    playerTracker.setOriginalPlayerId(player_id);
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
    if (start_mode == StartMode::AUTO_CREATE) {
        uint32_t gid = 0, pid = 0;
        uint8_t mapId = 0;
        bool ok = active_handler_->create_game_blocking(gid, pid, mapId, auto_create_game_name);
        if (ok)
        {
            my_game_id = gid;
            playerTracker.setPlayerId(static_cast<int32_t>(pid));
            playerTracker.setOriginalPlayerId(static_cast<int32_t>(pid));
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
        uint32_t pid = 0;
        uint8_t mapId = 0;
        bool ok = active_handler_->join_game_blocking(auto_join_game_id, pid, mapId);
        if (ok)
        {
            my_game_id = static_cast<uint32_t>(auto_join_game_id);
            playerTracker.setPlayerId(static_cast<int32_t>(pid));
            playerTracker.setOriginalPlayerId(static_cast<int32_t>(pid));
        }
        else
        {
            std::cout << "[Client] Failed to auto-join game " << auto_join_game_id << std::endl;
            connected = false;
            return;
        }
    }

    GameStateTracker stateManager;
    PositionUpdateHandler positionHandler;

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
                uint32_t gid = 0, pid = 0;
                uint8_t mapId = 0;
                bool ok = active_handler_->create_game_blocking(gid, pid, mapId);
                if (ok)
                {
                    my_game_id = gid;
                    playerTracker.setPlayerId(static_cast<int32_t>(pid));
                    playerTracker.setOriginalPlayerId(static_cast<int32_t>(pid));
                }
                else
                {
                    std::cout << "[Client] Failed to create game." << std::endl;
                }
            }
            else if (input.rfind(JOIN_GAME_STR, 0) == 0)
            {
                size_t last_space = input.find_last_of(' ');
                if (last_space != std::string::npos && last_space + 1 < input.size())
                {
                    std::string game_id_str = input.substr(last_space + 1);
                    try
                    {
                        int gid = std::stoi(game_id_str);
                        uint32_t pid = 0;
                        uint8_t mapId = 0;
                        bool ok = active_handler_->join_game_blocking(gid, pid, mapId);
                        if (ok)
                        {
                            my_game_id = static_cast<uint32_t>(gid);
                            playerTracker.setPlayerId(static_cast<int32_t>(pid));
                            playerTracker.setOriginalPlayerId(static_cast<int32_t>(pid));
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
                active_handler_->send(input);
            }
        }

        stateManager.resetFrameState();

        ServerMessage message;
        ServerMessage latest_message;
        bool got_message = false;
        uint8_t latest_opcode = 0;

        while (active_handler_->try_receive(message))
        {
            if (stateManager.shouldExitGame(message.opcode))
            {
                connected = false;
                return;
            }

            stateManager.onMessage(message);

            latest_message = message;
            got_message = true;
            latest_opcode = latest_message.opcode;
        }

        if (stateManager.shouldTriggerCountdown(got_message, latest_opcode))
        {
            game_renderer.startCountDown();
        }

        if (stateManager.shouldShowResults())
        {
            playerTracker.setPlayerId(playerTracker.getOriginalPlayerId());

            uint8_t upgrade_speed = 0;
            uint8_t upgrade_acceleration = 0;
            uint8_t upgrade_handling = 0;
            uint8_t upgrade_durability = 0;

            if (latest_message.opcode == UPDATE_POSITIONS) {
                for (const auto& pos : latest_message.positions) {
                    if (pos.player_id == playerTracker.getOriginalPlayerId()) {
                        upgrade_speed = pos.upgrade_speed;
                        upgrade_acceleration = pos.upgrade_acceleration;
                        upgrade_handling = pos.upgrade_handling;
                        upgrade_durability = pos.upgrade_durability;
                        break;
                    }
                }
            }

            game_renderer.winSound();
            game_renderer.showResults(
                stateManager.hasSawRaceTimes() ? stateManager.getLastRaceTimesMsg().race_times : std::vector<ServerMessage::PlayerRaceTime>(),
                stateManager.hasSawTotalTimes() ? stateManager.getLastTotalTimesMsg().total_times : std::vector<ServerMessage::PlayerTotalTime>(),
                playerTracker.getOriginalPlayerId(),
                upgrade_speed,
                upgrade_acceleration,
                upgrade_handling,
                upgrade_durability
            );
        }

        if (got_message && latest_message.opcode == UPDATE_POSITIONS && !latest_message.positions.empty())
        {
            auto playerInfo = playerTracker.findPlayerInPositions(latest_message);
            size_t idx_main = playerInfo.idx_main;
            bool player_found = playerInfo.player_found;

            auto deathState = playerTracker.checkDeathState(
                player_found,
                game_renderer.mainCar != nullptr,
                game_renderer.mainCar && game_renderer.mainCar->isExploding(),
                game_renderer.mainCar && game_renderer.mainCar->isExplosionComplete()
            );

            if (deathState == PlayerStateTracker::DeathState::NEEDS_EXPLOSION)
            {
                game_renderer.triggerPlayerDeath();
            }
            else if (deathState == PlayerStateTracker::DeathState::TRANSITION_COMPLETE)
            {
                idx_main = 0;
                player_found = true;
                game_renderer.completePlayerDeathTransition();
                playerTracker.switchToSpectatorMode();
            }

            auto frame = positionHandler.processUpdate(
                latest_message,
                idx_main,
                player_found,
                playerTracker.getOriginalPlayerId(),
                car_type_map,
                game_renderer.mainCar
            );

            game_renderer.updateResultsUpgrades(
                frame.upgradeSpeed,
                frame.upgradeAcceleration,
                frame.upgradeHandling,
                frame.upgradeDurability
            );

            game_renderer.render(
                frame.mainCarPosition,
                frame.mainTypeId,
                frame.otherCars,
                frame.next_cps,
                frame.mainCarCollisionFlag,
                frame.mainCarIsStopping,
                frame.mainCarHP,
                frame.otherCarsCollisionFlags,
                frame.otherCarsIsStoppingFlags
            );
        }

        SDL_Delay(16);
    }
}