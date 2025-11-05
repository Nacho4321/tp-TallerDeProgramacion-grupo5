#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include "../common/queue.h"
#include "game_monitor.h"
#include "acceptor.h"

class Server
{
private:
    Queue<int> clientes;
    OutboxMonitor outboxes;
    Queue<ClientHandlerMessage> global_inbox;
    GameMonitor games_monitor;
    Acceptor acceptor;
    void process_input(const std::string &input, bool &connected);

public:
    explicit Server(const char *port)
        : clientes(), outboxes(), global_inbox(), games_monitor(),
          acceptor(port, global_inbox, clientes, outboxes)
    {
    }
    void start();
};

#endif
