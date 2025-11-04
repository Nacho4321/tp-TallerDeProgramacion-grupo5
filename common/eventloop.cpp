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
            ClientHandlerMessage event = global_inbox.pop();
            Event ev = Event{event.client_id, event.msg.cmd};
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
    global_inbox.close();
    Thread::stop();
}