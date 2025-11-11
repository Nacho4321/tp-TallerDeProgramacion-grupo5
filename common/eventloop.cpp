#include "eventloop.h"
#include <queue>
#include <mutex>
#include <condition_variable>

void EventLoop::run()
{
    while (should_keep_running())
    {
        try
        {
            Event ev = event_queue->pop();
            dispatcher.handle_event(ev);
        }
        catch (const ClosedQueue &)
        {
            break;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error en event loop: " << e.what() << std::endl;
        }
    }
}

void EventLoop::stop()
{
    event_queue->close();
    Thread::stop();
}