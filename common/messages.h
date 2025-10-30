#ifndef MESSAGES_H
#define MESSAGES_H
#include "Event.h"
#include <string>

// Mensaje que el server va a manejar en su loop
struct PlayerPositionUpdate
{
    int player_id;
    Position new_pos;
};

struct ServerMessage 
{
    std::vector<PlayerPositionUpdate> positions;
};

struct ClientMessage 
{
    std::string cmd;
};
#endif
