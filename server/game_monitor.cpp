#include "game_monitor.h"

int GameMonitor::add_game(int &client_id, const std::string& name)
{
    std::lock_guard<std::mutex> lock1(games_mutex);
    std::lock_guard<std::mutex> lock2(game_queues_mutex);

    int game_id = next_id++;
    auto new_queue = std::make_shared<Queue<Event>>();
    games_queues[game_id] = new_queue;

    auto new_game = std::make_unique<GameLoop>(new_queue);

    auto player_outbox = outboxes.get_cliente_queue(client_id);
    if (!player_outbox) {
        throw std::runtime_error("Outbox not found for creator client");
    }
    new_game->add_player(client_id, player_outbox);
    new_game->start();

    games[game_id] = std::move(new_game);
    game_names[game_id] = name.empty() ? (std::string{"Game "} + std::to_string(game_id)) : name;

    std::cout << "GameMonitor: juego " << game_id << " creado para cliente " << client_id << std::endl;
    
    return game_id; // Devolver el game_id asignado
}

std::vector<ServerMessage::GameSummary> GameMonitor::list_games() {
    std::lock_guard<std::mutex> lock(games_mutex);
    std::vector<ServerMessage::GameSummary> result;
    result.reserve(games.size());
    for (auto &entry : games) {
        int gid = entry.first;
        auto &loopPtr = entry.second;
        uint32_t count = 0;
        if (loopPtr) {
            // Necesitamos método para obtener cantidad de jugadores
            // Agregaremos get_player_count() a GameLoop
            count = static_cast<uint32_t>(loopPtr->get_player_count());
        }
        ServerMessage::GameSummary summary{static_cast<uint32_t>(gid), game_names[gid], count};
        result.push_back(std::move(summary));
    }
    return result;
}

void GameMonitor::join_player(int &player_id, int &game_id)
{
    std::lock_guard<std::mutex> lock1(games_mutex);
    auto it = games.find(game_id);
    if (it == games.end() || !it->second) {
        throw std::runtime_error("Game not found");
    }
    auto q = outboxes.get_cliente_queue(player_id);
    if (!q) {
        throw std::runtime_error("Outbox not found for joining client");
    }
    std::cout << "[GameMonitor] join_player: game_id=" << game_id
              << " player_id=" << player_id
              << " outbox.valid=" << std::boolalpha << static_cast<bool>(q) << std::endl;
    it->second->add_player(player_id, q);
    std::cout << "[GameMonitor] join_player: add_player() OK" << std::endl;
}

void GameMonitor::remove_player(int client_id)
{
    std::lock_guard<std::mutex> lock(games_mutex);
    for (auto &entry : games)
    {
        auto &game = entry.second;
        if (!game)
            continue;
        // Intentar remover en cada juego (si existe). GameLoop::remove_player
        // internamente verificará si el player está presente.
        try
        {
            game->remove_player(client_id);
            // si no lanzó, asumimos que se removió y devolvemos
            std::cout << "[GameMonitor] remove_player: client " << client_id << " removed from game " << entry.first << std::endl;
            return;
        }
        catch (...) {
            // Ignorar y seguir buscando
        }
    }
    // Si no se encontró el jugador en ningún juego, no es fatal.
    std::cout << "[GameMonitor] remove_player: client " << client_id << " not found in any game" << std::endl;
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