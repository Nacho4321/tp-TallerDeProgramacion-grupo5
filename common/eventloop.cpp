#include "eventloop.h"
#include <queue>
#include <mutex>
#include <condition_variable>

// Procesa todos los eventos disponibles en la cola sin bloquear
void EventLoop::process_available_events()
{
    Event ev;
    // Procesar todos los eventos disponibles usando try_pop
    while (event_queue->try_pop(ev))
    {
        try
        {
            dispatcher.handle_event(ev);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[EventLoop] Error procesando evento: " << e.what() << std::endl;
        }
    }
}