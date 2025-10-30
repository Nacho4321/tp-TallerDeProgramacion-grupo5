#include "server.h"

#include <thread>
#define CLOSE_SERVER "q"

void Server::start()
{
    Event e1 = Event(1, MOVE_UP_PRESSED_STR);
    Event e2 = Event(1, MOVE_UP_RELEASED_STR);
    event_queue.push(e1);
    event_queue.push(e2);
    acceptor.start();
    need_for_speed.start();
    std::string input;
    bool connected = true;
    while (connected)
    {
        std::getline(std::cin, input);
        process_input(input, connected);
    }
    try
    {
        acceptor.stop();
        acceptor.join();
        need_for_speed.stop();
        need_for_speed.join();
    }
    catch (...)
    {
    }
}

void Server::process_input(const std::string &input, bool &connected)
{
    if (input == CLOSE_SERVER)
    {
        connected = false;
    }
    if (input == "start")
    {
        need_for_speed.start_game();
    }
}
