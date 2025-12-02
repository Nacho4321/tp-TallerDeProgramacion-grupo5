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
    bool success;
    uint8_t map_id = 0;
};

// ============================================
// Game messages (durante partida)
// ============================================

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
    
    // Niveles de mejora (0-3 cada uno)
    uint8_t upgrade_speed = 0;
    uint8_t upgrade_acceleration = 0;
    uint8_t upgrade_handling = 0;
    uint8_t upgrade_durability = 0;
    // Flag de frenazo 
    bool is_stopping = false;
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
    uint8_t map_id = 0; 

    // Payload para listado de partidas (GAMES_LIST)
    struct GameSummary
    {
        uint32_t game_id;
        std::string name;
        uint32_t player_count;
        uint8_t map_id = 0;
    };
    std::vector<GameSummary> games; // sólo usado si opcode == GAMES_LIST

    // Payload para resultados por carrera (RACE_TIMES)
    struct PlayerRaceTime {
        uint32_t player_id;
        uint32_t time_ms;
        bool disqualified;
        uint8_t round_index; // 0..2
    };
    std::vector<PlayerRaceTime> race_times; // usado si opcode == RACE_TIMES

    // Payload para totales del campeonato (TOTAL_TIMES)
    struct PlayerTotalTime {
        uint32_t player_id;
        uint32_t total_ms;
    };
    std::vector<PlayerTotalTime> total_times; // usado si opcode == TOTAL_TIMES
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
    uint8_t map_id = 0;
};
#endif
