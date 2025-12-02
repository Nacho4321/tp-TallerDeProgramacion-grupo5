#include "eventloop.h"
#include <queue>
#include <mutex>
#include <condition_variable>

EventLoop::EventLoop(std::mutex &map_mutex, std::unordered_map<int, PlayerData> &map, std::shared_ptr<Queue<Event>> &global_inb)
    : players_map_mutex(map_mutex), players(map), event_queue(global_inb), dispatcher(players_map_mutex, players)
{
}

void EventLoop::process_available_events(GameState state)
{
    Event ev;
    while (event_queue->try_pop(ev))
    {
        try
        {
            dispatcher.set_game_state(state);
            dispatcher.handle_event(ev);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[EventLoop] Error procesando evento: " << e.what() << std::endl;
        }
    }
}