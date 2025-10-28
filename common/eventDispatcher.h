#include <iostream>
#include <unordered_map>
#include <memory>
#include <functional>
#include "Event.h"

#define MOVE_FORWARD 0

class EventDispatcher
{
private:
    std::unordered_map<uint8_t, std::function<void(std::shared_ptr<Event>)>> listeners;
    void init_handlers();
    void move_forward(std::shared_ptr<Event> event);

public:
    EventDispatcher()
    {
        init_handlers();
    }

    void handle_event(std::shared_ptr<Event> event);
};
