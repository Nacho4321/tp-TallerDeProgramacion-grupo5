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
                ServerMessage msg; msg.opcode = UPDATE_POSITIONS; msg.positions = broadcast;

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

void GameLoop::add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    std::cout << "[GameLoop] add_player: id=" << id
              << " players.size()=" << players.size()
              << " outbox.valid=" << std::boolalpha << static_cast<bool>(player_outbox)
              << std::endl;
    std::vector<PlayerPositionUpdate> broadcast;
    if (int(players.size()) == 0)
    {
        players[id] = PlayerData{
            MOVE_UP_RELEASED_STR, CarInfo{"lambo", INITIAL_SPEED, INITIAL_SPEED, INITIAL_SPEED}, Position{INITIAL_X_POS, INITIAL_Y_POS, not_horizontal, not_vertical}};
        players_messanger[id] = player_outbox;
    }
    else if (int(players.size()) < MAX_PLAYERS)
    {
        std::cout << "[GameLoop] add_player: computing spawn near an existing player" << std::endl;
        auto anchor_it = players.begin();
        float dir_x = INITIAL_X_POS;
        float dir_y = INITIAL_Y_POS;
        if (anchor_it != players.end()) {
            std::cout << "[GameLoop] add_player: anchor id=" << anchor_it->first << std::endl;
            dir_x = anchor_it->second.position.new_X + 30.0f * static_cast<float>(players.size());
            dir_y = anchor_it->second.position.new_Y;
        }
        std::cout << "[GameLoop] add_player: spawn at (" << dir_x << "," << dir_y << ")" << std::endl;
        players[id] = PlayerData{
            MOVE_UP_RELEASED_STR, CarInfo{"lambo", INITIAL_SPEED, INITIAL_SPEED, INITIAL_SPEED}, Position{dir_x, dir_y, not_horizontal, not_vertical}};
        std::cout << "[GameLoop] add_player: player data inserted" << std::endl;
        players_messanger[id] = player_outbox;
        std::cout << "[GameLoop] add_player: messenger inserted" << std::endl;
    }
    else
    {
        std::cout << FULL_LOBBY_MSG << std::endl;
    }
    std::cout << "[GameLoop] add_player: done. players.size()=" << players.size()
              << " messengers.size()=" << players_messanger.size() << std::endl;
}

void GameLoop::start_game()
{
    started = true;
}

void GameLoop::broadcast_positions(ServerMessage &msg)
{
    // Snapshot de destinatarios para evitar iterar el mapa mientras puede cambiar
    std::vector<std::pair<int, std::shared_ptr<Queue<ServerMessage>>>> recipients;
    {
        std::lock_guard<std::mutex> lk(players_map_mutex);
        recipients.reserve(players_messanger.size());
        for (auto &entry : players_messanger) {
            recipients.emplace_back(entry.first, entry.second);
        }
    }

    std::vector<int> to_remove;
    for (auto &p : recipients)
    {
        int id = p.first;
        auto &queue = p.second;
        if (!queue) { to_remove.push_back(id); continue; }
        try {
            queue->push(msg);
        } catch (const ClosedQueue&) {
            // El cliente cerró su outbox: marcar para remover
            to_remove.push_back(id);
        }
    }
    if (!to_remove.empty()) {
        // Remover jugadores desconectados
        std::lock_guard<std::mutex> lk(players_map_mutex);
        for (int id : to_remove) {
            players_messanger.erase(id);
            players.erase(id);
        }
    }
}
