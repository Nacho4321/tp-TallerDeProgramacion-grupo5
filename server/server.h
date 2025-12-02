#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include "game_monitor.h"
#include "acceptor.h"
#include "lobby_handler.h"

class Server
{
private:
    GameMonitor games_monitor;
    LobbyHandler message_handler;
    Acceptor acceptor;
    void process_input(const std::string &input, bool &connected);

public:
    explicit Server(const char *port)
        : games_monitor(),
          message_handler(games_monitor), 
          acceptor(port, message_handler)
    {
    }
    void start();
};

#endif
