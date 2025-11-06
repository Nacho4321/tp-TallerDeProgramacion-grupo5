#include "game_monitor.h"

void GameMonitor::add_game(int &client_id)
{
    std::lock_guard<std::mutex> lock(games_mutex);
    auto new_queue = std::make_shared<Queue<Event>>();
    auto new_game = std::make_unique<GameLoop>(new_queue);
    games[next_id] = std::move(new_game);
    games[next_id]->start();
    games_queues[next_id] = new_queue;
    new_game->add_player(client_id, outboxes.get_cliente_queue(client_id));
    next_id++;
}

GameMonitor::~GameMonitor()
{
    for (auto &[id, game] : games)
    {
        game->stop();
        game->join();
    }
}