#ifndef EVENTLOOP_EVENTLOOP_H
#define EVENTLOOP_EVENTLOOP_H
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "queue.h"
#include "thread.h"
#include "eventDispatcher.h"

class EventLoop : public Thread
{
private:
    Queue<std::shared_ptr<Event>> &event_queue;
    EventDispatcher dispatcher;

public:
    explicit EventLoop(Queue<std::shared_ptr<Event>> &e_queue) : event_queue(e_queue), dispatcher() {}
    void run() override;
    void stop() override;
    ~EventLoop() override = default;
};
#endif