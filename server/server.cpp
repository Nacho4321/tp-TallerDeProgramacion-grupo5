#include "server.h"

#include <thread>
#define CLOSE_SERVER "q"

void Server::start()
{
    message_admin.start();
    acceptor.start();
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
        message_admin.stop();
        message_admin.join();
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
