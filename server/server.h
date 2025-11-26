#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include "game_monitor.h"
#include "acceptor.h"
#include "message_handler.h"

class Server
{
private:
    OutboxMonitor outboxes;  // Mantener por ahora para compatibilidad
    GameMonitor games_monitor;
    MessageHandler message_handler;
    Acceptor acceptor;
    void process_input(const std::string &input, bool &connected);

public:
    explicit Server(const char *port)
        : outboxes(), 
          games_monitor(),
          message_handler(games_monitor, outboxes), 
          acceptor(port, message_handler, outboxes)
    {
    }
    void start();
};

#endif
