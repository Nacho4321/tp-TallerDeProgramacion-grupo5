#ifndef EVENTLOOP_EVENTLOOP_H
#define EVENTLOOP_EVENTLOOP_H
#include <string>
#include "queue.h"
#include "thread.h"
#include "eventDispatcher.h"
#include "../server/client_handler_msg.h"
class EventLoop : public Thread
{
private:
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    Queue<ClientHandlerMessage> &global_inbox;
    EventDispatcher dispatcher;

public:
    explicit EventLoop(std::mutex &map_mutex, std::unordered_map<int, PlayerData> &map, Queue<ClientHandlerMessage> &global_inb) : players_map_mutex(map_mutex), players(map), global_inbox(global_inb), dispatcher(players_map_mutex, players) {}
    void run() override;
    void stop() override;
    ~EventLoop() override = default;
};
#endif