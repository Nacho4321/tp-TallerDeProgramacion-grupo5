#ifndef MESSAGES_H
#define MESSAGES_H
#include "position.h"
#include "constants.h"
#include <string>
#include <cstdint>
#include <variant>
#include <vector>

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
    // Tipo de auto actual de este jugador (para que el cliente elija sprite); para NPCs puede ser "npc".
    std::string car_type;
    // Up to N next checkpoints (in pixels) that the client can draw as guidance.
    // Coordinates are in the same units as Position (pixels).
    std::vector<Position> next_checkpoints;
    
    // HP system
    float hp = 100.0f;  //  HP actual 
    bool collision_flag = false;  // True si hubo colisión este frame (para animación de explosión)
};

// Mensaje unificado del servidor: puede ser una actualización de posiciones
// o una respuesta de lobby (por ejemplo, GAME_JOINED). Se indica con opcode.
struct ServerMessage
{
    // Debe coincidir con los opcodes definidos en constants.h (p.ej. UPDATE_POSITIONS, GAME_JOINED)
    uint8_t opcode = UPDATE_POSITIONS;

    // Payload para UPDATE_POSITIONS
    std::vector<PlayerPositionUpdate> positions;

    // Payload para GAME_JOINED (lobby)
    uint32_t game_id = 0;
    uint32_t player_id = 0;
    bool success = false;

    // Payload para listado de partidas (GAMES_LIST)
    struct GameSummary
    {
        uint32_t game_id;
        std::string name;
        uint32_t player_count;
    };
    std::vector<GameSummary> games; // sólo usado si opcode == GAMES_LIST
};

struct ClientMessage
{
    // Comando textual (movimientos: move_up_pressed, lobby: create_game, join_game <id>, etc.)
    std::string cmd;
    // Identificador del jugador asignado por el servidor. Antes de recibir GameJoinedResponse -> -1
    int32_t player_id = -1;
    // Identificador de la partida. Antes de estar dentro de una partida -> -1.
    // Para join_game se envía el ID objetivo aquí.
    int32_t game_id = -1;
    // Nombre de la partida (para create_game) u otro payload textual
    std::string game_name;
    // Tipo de auto solicitado en un cambio de auto (solo si cmd comienza con CHANGE_CAR_STR)
    std::string car_type;
};
#endif
