#include "eventDispatcher.h"

void EventDispatcher::init_handlers()
{
    listeners[MOVE_FORWARD] = [this](Event &e)
    { move_forward(e); };
}
void EventDispatcher::move_forward(Event &event)
{
    PlayerMovedEvent &pmEvent = dynamic_cast<PlayerMovedEvent &>(event);
    std::cout << "Player " << pmEvent.client_id
              << " se movió a X=" << pmEvent.new_X
              << " Y=" << pmEvent.new_Y << std::endl;
}

void EventDispatcher::handle_event(Event &event)
{
    auto it = listeners.find(event.action);
    if (it != listeners.end())
    {
        it->second(event);
    }
    else
    {
        std::cout << "Evento desconocido " << std::endl;
    }
}