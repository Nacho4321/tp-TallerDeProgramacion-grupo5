#ifndef CLIENT_HANDLER_MSG_H
#define CLIENT_HANDLER_MSG_H

#include "../common/messages.h"
#include "../common/queue.h"
#include <memory>

struct ClientHandlerMessage
{
    int client_id;
    ClientMessage msg;
    int game_id;
    std::shared_ptr<Queue<ServerMessage>> outbox;  // Cola de salida del cliente
};

#endif
