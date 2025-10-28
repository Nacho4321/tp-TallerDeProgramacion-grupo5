#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <atomic>
#include <list>
#include <mutex>
#include <string>

#include "../common/queue.h"
#include "../common/gameloop.h"
class Server
{
private:
    GameLoop need_for_speed;
    Queue<std::shared_ptr<Event>> event_queue;

    void process_input(const std::string &input, bool &connected);

public:
    explicit Server() : need_for_speed(event_queue) {}
    void start();
};

#endif
