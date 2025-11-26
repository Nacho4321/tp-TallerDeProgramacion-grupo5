#ifndef GAME_MONITOR_H
#define GAME_MONITOR_H
#include <unordered_map>
#include <memory>
#include <string>
#include "../common/gameloop.h"
#include <mutex>
#define STARTING_ID 1
class GameMonitor
{
private:
    std::unordered_map<int, std::unique_ptr<GameLoop>> games;
    std::mutex games_mutex;
    std::unordered_map<int, std::string> game_names; // nombres de partidas
    std::unordered_map<int, std::shared_ptr<Queue<Event>>> &games_queues;
    std::mutex &game_queues_mutex;
    int next_id;

public:
    ~GameMonitor();
    explicit GameMonitor(std::unordered_map<int, std::shared_ptr<Queue<Event>>> &game_qs, std::mutex &game_qs_mutex) : games(), games_mutex(), games_queues(game_qs), game_queues_mutex(game_qs_mutex), next_id(STARTING_ID) {}
    int add_game(int client_id, std::shared_ptr<Queue<ServerMessage>> player_outbox, const std::string& name = ""); // Devuelve el game_id asignado
    void join_player(int player_id, int game_id, std::shared_ptr<Queue<ServerMessage>> player_outbox);
    std::vector<ServerMessage::GameSummary> list_games();
    GameLoop* get_game(int game_id); // Para acceso directo al GameLoop
};

#endif