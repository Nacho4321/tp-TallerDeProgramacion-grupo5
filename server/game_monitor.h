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
    std::mutex games_mutex;
    int next_id;

public:
    ~GameMonitor();
    explicit GameMonitor() : games(), games_mutex(), next_id(STARTING_ID) {}
    void add_game(std::unique_ptr<GameLoop> game);
};

#endif