#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include "../common/queue.h"
#include "../common/gameloop.h"
#include "acceptor.h"

class Server
{
private:
    Queue<int> clientes;
    OutboxMonitor outboxes;
    Queue<Event> event_queue;
    Queue<ClientMessage> global_inbox;
    GameLoop need_for_speed;
    Acceptor acceptor;
    void process_input(const std::string &input, bool &connected);

public:
    explicit Server(const char *port)
        : clientes(), outboxes(), global_inbox(), need_for_speed(event_queue, clientes, global_inbox, outboxes),
          acceptor(port, global_inbox, clientes, outboxes)
    {
    }
    void start();
};

#endif
