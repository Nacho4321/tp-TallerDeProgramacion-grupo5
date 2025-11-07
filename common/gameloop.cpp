#include "gameloop.h"
#include <thread>
#include <chrono>
#define MAX_PLAYERS 8
#define INITIAL_X_POS 960
#define INITIAL_Y_POS 540
// Velocidad base en pixeles por segundo (antes era 0.0008 por tick, casi imperceptible)
#define INITIAL_SPEED 200.0f
#define FULL_LOBBY_MSG "can't join lobby, maximum players reached"
void GameLoop::run()
{
    event_loop.start();
    auto last_tick = std::chrono::steady_clock::now();
    while (should_keep_running())
    {
        try {
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - last_tick).count(); // segundos
            last_tick = now;
            if (int(players.size()) > 0)
            {
                std::vector<PlayerPositionUpdate> broadcast;
                players_map_mutex.lock();
                for (auto &[id, player_data] : players)
                {
                    Position &pos = player_data.position;
                    // Movimiento en función del tiempo: pixeles/segundo * segundos
                    pos.new_X += float(pos.direction_x) * player_data.car.speed * dt;
                    pos.new_Y += float(pos.direction_y) * player_data.car.speed * dt;
                    PlayerPositionUpdate update = PlayerPositionUpdate{id, pos};
                    broadcast.push_back(update);
                }
                players_map_mutex.unlock();
                ServerMessage msg = {broadcast};

                broadcast_positions(msg);
            }
        } catch (const ClosedQueue&) {
            // Ignorar: alguna cola de cliente cerrada; ya se limpia en broadcast_positions
        } catch (const std::exception& e) {
            std::cerr << "[GameLoop] Unexpected exception: " << e.what() << std::endl;
        }

        // Throttle tick rate to ~60 FPS to avoid flooding network/CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    event_loop.stop();
    event_loop.join();
}

void GameLoop::add_player(int id, std::shared_ptr<Queue<ServerResponse>> player_outbox)
{
    std::vector<PlayerPositionUpdate> broadcast;
    if (int(players.size()) == 0)
    {
        players[id] = PlayerData{
            MOVE_UP_RELEASED_STR, CarInfo{"lambo", INITIAL_SPEED, INITIAL_SPEED, INITIAL_SPEED}, Position{INITIAL_X_POS, INITIAL_Y_POS, not_horizontal, not_vertical}};
        players_messanger[id] = player_outbox;
    }
    else if (int(players.size()) < MAX_PLAYERS)
    {
        auto last_it = std::prev(players.end());
        float dir_x = last_it->second.position.new_X + 30;
        float dir_y = last_it->second.position.new_Y;
        players[id] = PlayerData{
            MOVE_UP_RELEASED_STR, CarInfo{"lambo", INITIAL_SPEED, INITIAL_SPEED, INITIAL_SPEED}, Position{dir_x, dir_y, not_horizontal, not_vertical}};
        players_messanger[id] = player_outbox;
    }
    else
    {
        std::cout << FULL_LOBBY_MSG << std::endl;
    }
}

void GameLoop::start_game()
{
    started = true;
}

void GameLoop::broadcast_positions(ServerMessage &msg)
{
    std::vector<int> to_remove;
    for (auto &[id, queue] : players_messanger)
    {
        try {
            queue->push(ServerResponse{msg}); // Wrap en ServerResponse
        } catch (const ClosedQueue&) {
            // El cliente cerró su outbox: marcar para remover
            to_remove.push_back(id);
        }
    }
    if (!to_remove.empty()) {
        // Remover jugadores desconectados
        for (int id : to_remove) {
            players_messanger.erase(id);
            // Opcionalmente remover del mapa de jugadores
            std::lock_guard<std::mutex> lk(players_map_mutex);
            players.erase(id);
        }
    }
}
