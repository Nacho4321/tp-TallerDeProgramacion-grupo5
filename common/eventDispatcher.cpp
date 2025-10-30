#include "eventDispatcher.h"

void EventDispatcher::init_handlers()
{
    listeners[MOVE_UP_PRESSED] = [this](Event &e)
    { move_forward(e); };
    listeners[MOVE_UP_RELEASED] = [this](Event &e)
    { move_released(e); };
}
void EventDispatcher::move_forward(Event &event)
{
    PlayerMovedEvent mover{event.client_id, event.action, 10.0, 20.0, forward, forward};
    std::cout << "Jugador se movio para adelante" << std::endl;
}

void EventDispatcher::move_released(Event &event)
{
    PlayerMovedEvent mover{event.client_id, event.action, 3.0, 0, forward, forward};
    std::cout << "Jugador dejo de moverse para adelante" << std::endl;
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