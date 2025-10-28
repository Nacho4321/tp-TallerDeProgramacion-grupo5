#include "server.h"

#include <thread>
#define CLOSE_SERVER "q"

void Server::start()
{
    auto e1 = std::make_shared<PlayerMovedEvent>(1, MOVE_FORWARD, 10.0, 20.0);
    auto e2 = std::make_shared<PlayerMovedEvent>(2, MOVE_FORWARD, 15.5, 22.3);
    event_queue.push(e1);
    event_queue.push(e2);

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
}
