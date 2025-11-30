#include "game_monitor.h"

GameMonitor::GameMonitor()
    : games(), games_queues(), game_names(), games_mutex(), next_id(STARTING_ID)
{
}

int GameMonitor::add_game(int client_id, std::shared_ptr<Queue<ServerMessage>> player_outbox, const std::string& name)
{
    std::lock_guard<std::mutex> lock(games_mutex);  // Un solo lock

    int game_id = next_id++;
    auto new_queue = std::make_shared<Queue<Event>>();
    games_queues[game_id] = new_queue;

    auto new_game = std::make_unique<GameLoop>(new_queue);

    if (!player_outbox) {
        throw std::runtime_error("Outbox not found for creator client");
    }
    new_game->add_player(client_id, player_outbox);
    new_game->start();

    games[game_id] = std::move(new_game);
    game_names[game_id] = name.empty() ? (std::string{"Game "} + std::to_string(game_id)) : name;

    std::cout << "GameMonitor: juego " << game_id << " creado para cliente " << client_id << std::endl;
    
    return game_id;
}

std::vector<ServerMessage::GameSummary> GameMonitor::list_games() {
    std::lock_guard<std::mutex> lock(games_mutex);
    std::vector<ServerMessage::GameSummary> result;
    result.reserve(games.size());
    for (auto &entry : games) {
        int gid = entry.first;
        auto &loopPtr = entry.second;
        
        // Solo mostrar partidas que estén en estado LOBBY (joinable)
        if (!loopPtr || !loopPtr->is_joinable()) {
            continue;
        }
        
        uint32_t count = static_cast<uint32_t>(loopPtr->get_player_count());
        ServerMessage::GameSummary summary{static_cast<uint32_t>(gid), game_names[gid], count};
        result.push_back(std::move(summary));
    }
    return result;
}

void GameMonitor::join_player(int player_id, int game_id, std::shared_ptr<Queue<ServerMessage>> player_outbox)
{
    std::lock_guard<std::mutex> lock(games_mutex);  // Un solo lock
    auto it = games.find(game_id);
    if (it == games.end() || !it->second) {
        throw std::runtime_error("Game not found");
    }
    if (!player_outbox) {
        throw std::runtime_error("Outbox not found for joining client");
    }
    std::cout << "[GameMonitor] join_player: game_id=" << game_id
              << " player_id=" << player_id
              << " outbox.valid=" << std::boolalpha << static_cast<bool>(player_outbox) << std::endl;
    it->second->add_player(player_id, player_outbox);
    std::cout << "[GameMonitor] join_player: add_player() OK" << std::endl;
}

std::shared_ptr<Queue<Event>> GameMonitor::get_game_queue(int game_id) {
    std::lock_guard<std::mutex> lock(games_mutex);
    auto it = games_queues.find(game_id);
    if (it != games_queues.end()) {
        return it->second;
    }
    return nullptr;
}

GameLoop* GameMonitor::get_game(int game_id) {
    std::lock_guard<std::mutex> lock(games_mutex);
    auto it = games.find(game_id);
    if (it == games.end() || !it->second) {
        return nullptr;
    }
    return it->second.get();
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