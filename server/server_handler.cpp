#include "server_handler.h"

ServerHandler::ServerHandler(const char *port)
    : inbox(), outboxes(),
      acceptor(port, inbox, outboxes) {}

ServerHandler::~ServerHandler()
{
    stop();
}

void ServerHandler::start()
{
    if (!started)
    {
        acceptor.start();
        started = true;
    }
}

void ServerHandler::stop()
{
    if (started)
    {
        acceptor.stop();
        acceptor.join();
        started = false;
    }
}

bool ServerHandler::try_receive(ClientHandlerMessage &out)
{
    return inbox.try_pop(out);
}

void ServerHandler::broadcast(const ServerMessage &msg)
{
    acceptor.broadcast(msg);
}
