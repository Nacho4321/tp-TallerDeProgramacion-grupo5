#ifndef CLIENT_HANDLER_MSG_H
#define CLIENT_HANDLER_MSG_H

#include "../common/messages.h"

struct ClientHandlerMessage
{
    int client_id;
    ClientMessage msg;
    int game_id;
};

#endif
