#include "eventDispatcher.h"

void EventDispatcher::init_handlers()
{
    listeners[MOVE_UP_PRESSED_STR] = [this](Event &e)
    { move_up(e); };
    listeners[MOVE_UP_RELEASED_STR] = [this](Event &e)
    { move_up_released(e); };
    listeners[MOVE_DOWN_PRESSED_STR] = [this](Event &e)
    { move_down(e); };
    listeners[MOVE_DOWN_RELEASED_STR] = [this](Event &e)
    { move_down_released(e); };
    listeners[MOVE_LEFT_PRESSED_STR] = [this](Event &e)
    { move_left(e); };
    listeners[MOVE_LEFT_RELEASED_STR] = [this](Event &e)
    { move_left_released(e); };
    listeners[MOVE_RIGHT_PRESSED_STR] = [this](Event &e)
    { move_right(e); };
    listeners[MOVE_RIGHT_RELEASED_STR] = [this](Event &e)
    { move_right_released(e); };
}

void EventDispatcher::move_up(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_y = up;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_up_released(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_y = not_vertical;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_down(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_y = down;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_down_released(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_y = not_vertical;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_left(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_x = left;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_left_released(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_x = not_horizontal;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_right(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_x = right;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_right_released(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_x = not_horizontal;
    players[event.client_id].state = event.action;
}
void EventDispatcher::handle_event(Event &event)
{
    auto it = listeners.find(event.action);
    if (it != listeners.end())
    {
        it->second(event);
    }
    else
    {
        std::cout << "Evento desconocido: " << event.action << std::endl;
    }
}