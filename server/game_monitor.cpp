#include "game_monitor.h"

void GameMonitor::add_game(std::unique_ptr<GameLoop> game)
{
    std::lock_guard<std::mutex> lock(games_mutex);
    int new_game_id = next_id++;
    games[new_game_id] = std::move(game);
}
