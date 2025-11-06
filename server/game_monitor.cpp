#include "game_monitor.h"

void GameMonitor::add_game(int &client_id)
{
    std::lock_guard<std::mutex> lock(games_mutex);
    auto new_queue = std::make_shared<Queue<Event>>();
    auto new_game = std::make_unique<GameLoop>(new_queue);
    games[next_id] = std::move(new_game);
    games[next_id]->start();
    games_queues[next_id] = new_queue;
    join_player(client_id, next_id);
    next_id++;
}

void GameMonitor::join_player(int &player_id, int &game_id)
{
    games[game_id]->add_player(player_id, outboxes.get_cliente_queue(player_id));
}
GameMonitor::~GameMonitor()
{
    for (auto &[id, game] : games)
    {
        game->stop();
        game->join();
    }
}