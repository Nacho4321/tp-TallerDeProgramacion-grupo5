#ifndef MESSAGES_H
#define MESSAGES_H
#include "Event.h"
#include <string>

// Mensaje que el server va a manejar en su loop
struct OutgoingMessage
{
    int player_id;
    Position new_pos;
};

struct IncomingMessage
{
    std::string cmd;
    int client_id;
};
#endif
