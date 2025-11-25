#include <iostream>
#include <unordered_map>
#include <functional>
#include <unordered_map>
#include <mutex>
#include "Event.h"
#include "constants.h"
#include "PlayerData.h"
#include "car_physics_config.h"
class EventDispatcher
{
private:
    std::unordered_map<std::string, std::function<void(Event &)>> listeners;
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    void init_handlers();
    void move_up(Event &event);
    void move_up_released(Event &event);
    void move_down(Event &event);
    void move_down_released(Event &event);
    void move_left(Event &event);
    void move_left_released(Event &event);
    void move_right(Event &event);
    void move_right_released(Event &event);
    void change_car(Event &event, const std::string& car_type);

public:
    EventDispatcher(std::mutex &map_mutex, std::unordered_map<int, PlayerData> &map) : players_map_mutex(map_mutex), players(map)
    {
        init_handlers();
    }

    void handle_event(Event &event);
};
