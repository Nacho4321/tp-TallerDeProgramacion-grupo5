#include "server.h"

#include <thread>
#define CLOSE_SERVER "q"

void Server::start()
{

    auto new_game = std::make_unique<GameLoop>(outboxes);
    games_monitor.add_game(std::move(new_game));
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
