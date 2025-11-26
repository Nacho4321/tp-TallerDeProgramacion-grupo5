#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include "game_monitor.h"
#include "acceptor.h"
#include "message_handler.h"

class Server
{
private:
    OutboxMonitor outboxes;
    std::unordered_map<int, std::shared_ptr<Queue<Event>>> game_queues;
    std::mutex games_queues_mutex;
    GameMonitor games_monitor;
    MessageHandler message_handler;
    Acceptor acceptor;
    void process_input(const std::string &input, bool &connected);

public:
    explicit Server(const char *port)
        : outboxes(), game_queues(), games_queues_mutex(), 
          games_monitor(game_queues, games_queues_mutex, outboxes),
          message_handler(game_queues, games_queues_mutex, games_monitor, outboxes), 
          acceptor(port, message_handler, outboxes)
    {
    }
    void start();
};

#endif
