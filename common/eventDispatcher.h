#include <iostream>
#include <unordered_map>
#include <memory>
#include <functional>
#include "Event.h"
#include "constants.h"

class EventDispatcher
{
private:
    std::unordered_map<uint8_t, std::function<void(Event &)>> listeners;
    void init_handlers();
    void move_forward(Event &event);
    void move_released(Event &event);

public:
    EventDispatcher()
    {
        init_handlers();
    }

    void handle_event(Event &event);
};
