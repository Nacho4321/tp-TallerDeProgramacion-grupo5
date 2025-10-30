#include <iostream>
#include <unordered_map>
#include <functional>
#include <unordered_map>
#include <mutex>
#include "Event.h"
#include "constants.h"
#include "PlayerData.h"
class EventDispatcher
{
private:
    std::unordered_map<std::string, std::function<void(Event &)>> listeners;
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    void init_handlers();
    void move_up(Event &event);
    void move_up_released(Event &event);

public:
    EventDispatcher(std::mutex &map_mutex, std::unordered_map<int, PlayerData> &map) : players_map_mutex(map_mutex), players(map)
    {
        init_handlers();
    }

    void handle_event(Event &event);
};
