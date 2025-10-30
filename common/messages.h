#ifndef MESSAGES_H
#define MESSAGES_H
#include "Event.h"
#include <string>

// Mensaje que el server va a manejar en su loop
struct ServerMessage
{
    int player_id;
    Position new_pos;
};

struct ClientMessage
{
    std::string cmd;
    int client_id;
};
#endif
