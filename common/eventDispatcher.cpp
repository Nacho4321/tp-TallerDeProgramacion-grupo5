#include "eventDispatcher.h"

void EventDispatcher::init_handlers()
{
    listeners[MOVE_FORWARD] = [this](std::shared_ptr<Event> e)
    { move_forward(e); };
}
void EventDispatcher::move_forward(std::shared_ptr<Event> event)
{
    if (auto pmEvent = std::dynamic_pointer_cast<PlayerMovedEvent>(event))
    {
        std::cout << "Player " << pmEvent->client_id
                  << " se movió a X=" << pmEvent->new_X
                  << " Y=" << pmEvent->new_Y << std::endl;
    }
    else
    {
        std::cerr << "Error: evento no es PlayerMovedEvent\n";
    }
}

void EventDispatcher::handle_event(std::shared_ptr<Event> event)
{
    auto it = listeners.find(event->action);
    if (it != listeners.end())
    {
        it->second(event);
    }
    else
    {
        std::cout << "Evento desconocido " << std::endl;
    }
}