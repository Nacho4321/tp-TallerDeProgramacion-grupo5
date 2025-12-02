#ifndef EVENTLOOP_EVENTLOOP_H
#define EVENTLOOP_EVENTLOOP_H
#include <string>
#include "../common/queue.h"
#include "eventDispatcher.h"
#include "game_state.h"

class EventLoop
{
private:
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    std::shared_ptr<Queue<Event>> &event_queue;
    EventDispatcher dispatcher;

public:
    explicit EventLoop(std::mutex &map_mutex, std::unordered_map<int, PlayerData> &map, std::shared_ptr<Queue<Event>> &global_inb);
    void process_available_events(GameState state);

    ~EventLoop() = default;
};
#endif