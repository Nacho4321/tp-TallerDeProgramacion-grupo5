#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include "game_monitor.h"
#include "acceptor.h"
#include "message_admin.h"

class Server
{
private:
    OutboxMonitor outboxes;
    Queue<ClientHandlerMessage> global_inbox;
    std::unordered_map<int, Queue<Event>> game_queues;
    std::mutex games_queues_mutex;
    GameMonitor games_monitor;
    MessageAdmin message_admin;
    Acceptor acceptor;
    void process_input(const std::string &input, bool &connected);

public:
    explicit Server(const char *port)
        : outboxes(), global_inbox(), game_queues(), games_queues_mutex(), games_monitor(game_queues, games_queues_mutex),
          message_admin(global_inbox, game_queues, games_queues_mutex), acceptor(port, global_inbox, outboxes)
    {
    }
    void start();
};

#endif
