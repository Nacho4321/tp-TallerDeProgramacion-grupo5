#ifndef CLIENT_H
#define CLIENT_H
#include "../common/protocol.h"
#include "../common/queue.h"
#include "input_handler.h"
#include "game_client_handler.h"
#include "game_renderer.h"
#include <SDL2pp/SDL2pp.hh>

class Client
{
private:
    Protocol protocol;
    Socket ini_protocol(const char *address, const char *port)
    {
        Socket sock(address, port);
        return sock;
    }
    bool connected;
    InputHandler handler;
    
    // Handler que encapsula sender/receiver y colas
    GameClientHandler handler_core;

    GameRenderer game_renderer;

public:
    explicit Client(const char *address, const char *port);
    ~Client();
    void start();
    bool isConnected() const { return connected; }
};
#endif