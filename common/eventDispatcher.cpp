#include "eventDispatcher.h"

void EventDispatcher::init_handlers()
{
    listeners[MOVE_UP_PRESSED_STR] = [this](Event &e)
    { move_up(e); };
    listeners[MOVE_UP_RELEASED_STR] = [this](Event &e)
    { move_up_released(e); };
}
void EventDispatcher::move_up(Event &event)
{
    players_map_mutex.lock();
    Position actual_pos = players[event.client_id].position;
    actual_pos.direction_y = up;
    actual_pos.new_Y = actual_pos.new_Y + 14;
    players[event.client_id].position = actual_pos;
    players[event.client_id].state = event.action;
    players_map_mutex.unlock();
}

void EventDispatcher::move_up_released(Event &event)
{
    players_map_mutex.lock();
    Position actual_pos = players[event.client_id].position;
    if (actual_pos.new_Y > 0)
    {
        actual_pos.new_Y = actual_pos.new_Y - 7;
        if (actual_pos.new_Y < 0)
        {
            actual_pos.new_Y = 0;
        }
    }
    players[event.client_id].position = actual_pos;
    players[event.client_id].state = event.action;
    players_map_mutex.unlock();
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
        std::cout << "Evento desconocido " << std::endl;
    }
}