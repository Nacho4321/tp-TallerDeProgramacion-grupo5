#ifndef GAME_MONITOR_H
#define GAME_MONITOR_H
#include <unordered_map>
#include <memory>
#include "../common/gameloop.h"
#include <mutex>
#define STARTING_ID 1
class GameMonitor
{
private:
    std::unordered_map<int, std::unique_ptr<GameLoop>> games;
    std::unordered_map<int, Queue<Event>> &game_queues;
    std::mutex games_mutex;
    std::mutex &game_queues_mutex;
    int next_id;

public:
    ~GameMonitor();
    explicit GameMonitor(std::unordered_map<int, Queue<Event>> &game_qs, std::mutex &game_qs_mutex) : games(), game_queues(game_qs), games_mutex(), game_queues_mutex(game_qs_mutex), next_id(STARTING_ID) {}
    void add_game(std::unique_ptr<GameLoop> game);
};

#endif