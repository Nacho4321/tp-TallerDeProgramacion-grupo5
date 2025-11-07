#include "game_monitor.h"

int GameMonitor::add_game(int &client_id)
{
    std::lock_guard<std::mutex> lock1(games_mutex);
    std::lock_guard<std::mutex> lock2(game_queues_mutex);

    int game_id = next_id++;
    auto new_queue = std::make_shared<Queue<Event>>();
    games_queues[game_id] = new_queue;

    auto new_game = std::make_unique<GameLoop>(new_queue);

    auto player_outbox = outboxes.get_cliente_queue(client_id);
    new_game->add_player(client_id, player_outbox);
    new_game->start();

    games[game_id] = std::move(new_game);

    std::cout << "GameMonitor: juego " << game_id << " creado para cliente " << client_id << std::endl;
    
    return game_id; // Devolver el game_id asignado
}

void GameMonitor::join_player(int &player_id, int &game_id)
{
    std::lock_guard<std::mutex> lock1(games_mutex);
    auto it = games.find(game_id);
    if (it == games.end() || !it->second) {
        throw std::runtime_error("Game not found");
    }
    it->second->add_player(player_id, outboxes.get_cliente_queue(player_id));
}
GameMonitor::~GameMonitor()
{
    std::lock_guard<std::mutex> lock(games_mutex);
    for (auto &[id, game] : games)
    {
        game->stop();
        game->join();
    }
}