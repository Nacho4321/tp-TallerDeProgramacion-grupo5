#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <atomic>
#include <list>
#include <mutex>
#include <string>

#include "../common/eventloop.h"
#include "../common/queue.h"
class Server
{
private:
    Queue<Event> event_queue;
    EventLoop need_for_speed;

    void process_input(const std::string &input, bool &connected);

public:
    explicit Server() : need_for_speed(event_queue) {}
    void start();
};

#endif
