#ifndef MESSAGES_H
#define MESSAGES_H
#include "Event.h"
#include <string>
#include <cstdint>
#include <variant>

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

// Wrapper que unifica los diferentes tipos de respuestas del servidor
using ServerResponse = std::variant<ServerMessage, GameJoinedResponse>;

struct ClientMessage
{
    // Comando textual (movimientos: move_up_pressed, lobby: create_game, join_game <id>, etc.)
    std::string cmd;
    // Identificador del jugador asignado por el servidor. Antes de recibir GameJoinedResponse -> -1
    int32_t player_id = -1;
    // Identificador de la partida. Antes de estar dentro de una partida -> -1.
    // Para join_game se envía el ID objetivo aquí.
    int32_t game_id = -1;
};
#endif
