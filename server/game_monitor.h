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
    std::unordered_map<int, std::shared_ptr<Queue<Event>>> games_queues;
    std::unordered_map<int, std::string> game_names; // nombres de partidas
    std::mutex games_mutex;  // Un solo mutex para proteger todo
    int next_id;

public:
    ~GameMonitor();
    explicit GameMonitor() : games(), games_queues(), game_names(), games_mutex(), next_id(STARTING_ID) {}
    int add_game(int client_id, std::shared_ptr<Queue<ServerMessage>> player_outbox, const std::string& name = ""); // Devuelve el game_id asignado
    void join_player(int player_id, int game_id, std::shared_ptr<Queue<ServerMessage>> player_outbox);
    std::vector<ServerMessage::GameSummary> list_games();
    GameLoop* get_game(int game_id); // Para acceso directo al GameLoop
    std::shared_ptr<Queue<Event>> get_game_queue(int game_id); // Para obtener la cola de eventos de un juego
};

#endif