#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H
#include "client_handler.h"
#include "../common/queue.h"
#include <unordered_map>
#include "../common/Event.h"
#include <functional>
#include "game_monitor.h"

class MessageHandler
{
private:
    GameMonitor &games_monitor;
    std::unordered_map<std::string, std::function<void(ClientHandlerMessage &)>> cli_comm_dispatch;
    OutboxMonitor &outboxes;
    
    void init_dispatch();
    void create_game(ClientHandlerMessage &message);
    void join_game(ClientHandlerMessage &message);
    void get_games(ClientHandlerMessage &message);
    void start_game(ClientHandlerMessage &message);
    void leave_game(ClientHandlerMessage &message);

public:
    explicit MessageHandler(GameMonitor &games_mon, OutboxMonitor &outbox);
    
    virtual ~MessageHandler() = default;
    
    // Procesa un mensaje del cliente 
    virtual void handle_message(ClientHandlerMessage &message);
};

#endif