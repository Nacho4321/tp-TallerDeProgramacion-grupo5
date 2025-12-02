#ifndef LOBBY_HANDLER_H
#define LOBBY_HANDLER_H
#include "client_handler.h"
#include "../common/queue.h"
#include <unordered_map>
#include "event.h"
#include <functional>
#include "game_monitor.h"

class LobbyHandler
{
private:
    GameMonitor &games_monitor;
    std::unordered_map<std::string, std::function<void(ClientHandlerMessage &)>> lobby_command_handlers;
    
    void init_dispatch();
    void create_game(ClientHandlerMessage &message);
    void join_game(ClientHandlerMessage &message);
    void get_games(ClientHandlerMessage &message);
    void start_game(ClientHandlerMessage &message);
    void leave_game(ClientHandlerMessage &message);

public:
    explicit LobbyHandler(GameMonitor &games_mon);
    
    virtual ~LobbyHandler() = default;
    
    // Procesa un mensaje del cliente 
    virtual void handle_message(ClientHandlerMessage &message);
};

#endif