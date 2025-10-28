#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <atomic>
#include <list>
#include <mutex>
#include <string>

#include "../common/eventloop.h"
#include "../common/queue.h"
#include "acceptor.h"
class Server
{
private:
    Queue<std::shared_ptr<Event>> event_queue;
    EventLoop need_for_speed;
    Queue<IncomingMessage> global_inbox;
    Acceptor acceptor;
    void process_input(const std::string &input, bool &connected);

public:
    explicit Server(const char *port)
        : need_for_speed(event_queue),
          global_inbox(),
          acceptor(port, global_inbox)
    {
    }
    void start();
};

#endif
