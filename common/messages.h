#ifndef MESSAGES_H
#define MESSAGES_H
#include "Event.h"
#include <string>
#include <cstdint>

// ============================================
// Lobby responses (servidor -> cliente)
// ============================================

struct GameJoinedResponse
{
    uint32_t game_id;
    uint32_t player_id;
    bool success; // true si se unió correctamente, false si hubo error
};

// ============================================
// Game messages (durante partida)
// ============================================

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
    std::string cmd; // Puede ser: movimientos, "create_game", "join_game 42", etc.
};
#endif
