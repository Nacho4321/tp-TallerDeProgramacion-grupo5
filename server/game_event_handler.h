#ifndef GAME_EVENT_HANDLER_H
#define GAME_EVENT_HANDLER_H

#include <iostream>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "event.h"
#include "../common/constants.h"
#include "PlayerData.h"
#include "car_physics_config.h"
#include "game_state.h"

class GameEventHandler
{
private:
    std::unordered_map<std::string, std::function<void(Event &)>> listeners;
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    GameState current_state{GameState::LOBBY};

    void init_handlers();

    void move_up(Event &event);
    void move_up_released(Event &event);
    void move_down(Event &event);
    void move_down_released(Event &event);
    void move_left(Event &event);
    void move_left_released(Event &event);
    void move_right(Event &event);
    void move_right_released(Event &event);

    void upgrade_max_speed(Event &event);
    void upgrade_max_acceleration(Event &event);
    void upgrade_durability(Event &event);
    void upgrade_handling(Event &event);

    void cheat_god_mode(Event &event);
    void cheat_die(Event &event);
    void cheat_skip_round(Event &event);
    void cheat_full_upgrade(Event &event);

    void select_car(Event &event, const std::string &car_type);

public:
    GameEventHandler(std::mutex &map_mutex, std::unordered_map<int, PlayerData> &map) : players_map_mutex(map_mutex), players(map)
    {
        init_handlers();
    }

    inline void set_game_state(GameState s) { current_state = s; }
    void handle_event(Event &event);
};

#endif