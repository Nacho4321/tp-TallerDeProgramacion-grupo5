#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <string>

const std::uint8_t MOVE_UP_PRESSED = 0x01;
const std::uint8_t MOVE_UP_RELEASED = 0x02;
const std::uint8_t MOVE_DOWN_PRESSED = 0x03;
const std::uint8_t MOVE_DOWN_RELEASED = 0x04;
const std::uint8_t MOVE_LEFT_PRESSED = 0x05;
const std::uint8_t MOVE_LEFT_RELEASED = 0x06;
const std::uint8_t MOVE_RIGHT_PRESSED = 0x07;
const std::uint8_t MOVE_RIGHT_RELEASED = 0x08;

// Lobby opcodes
const std::uint8_t CREATE_GAME = 0x10;
const std::uint8_t JOIN_GAME = 0x11;
const std::uint8_t GAME_JOINED = 0x12;
// Listar partidas existentes
const std::uint8_t GET_GAMES = 0x13;
const std::uint8_t GAMES_LIST = 0x14;
// Start game (transition from lobby to playing)
const std::uint8_t START_GAME = 0x15;
// Notificacion de que el juego comenzo
const std::uint8_t GAME_STARTED = 0x16;
// Notify clients that STARTING countdown began (no payload; client assumes duration)
// Mejora
const std::uint8_t STARTING_COUNTDOWN = 0x17;
const std::uint8_t UPGRADE_CAR = 0x18;
// Race timing results per round
const std::uint8_t RACE_TIMES = 0x40;
// Championship totals after 3 rounds
const std::uint8_t TOTAL_TIMES = 0x41;

// Game opcodes
const std::uint8_t UPDATE_POSITIONS = 0x20;

const int OPCODE_SIZE = 1;
const std::uint8_t POSITIONS_SIZE = 2;

// Radio de los checkpoints en píxeles que usan servidor y cliente para dibujar
constexpr float CHECKPOINT_RADIUS_PX = 20.0f;

// Display/Rendering constants - logical screen dimensions for aspect ratio preservation
constexpr int LOGICAL_SCREEN_WIDTH = 854;
constexpr int LOGICAL_SCREEN_HEIGHT = 480;

constexpr float DEFAULT_CAR_SPEED_PX_S = 200.0f;  // pixels/sec
constexpr float DEFAULT_CAR_ACCEL_PX_S2 = 400.0f; // px/sec^2
constexpr float DEFAULT_CAR_HP = 100.0f;

// NPCs
constexpr int MAX_MOVING_NPCS = 20;      // NPCs circulando por waypoints
constexpr int MAX_PARKED_NPCS = 10;      // NPCs estacionados
constexpr float NPC_SPEED_PX_S = 120.0f; // pixels/sec

// Car change (runtime swap during game)
constexpr uint8_t CHANGE_CAR = 0x30;             // opcode para cambio de auto
const std::string CHANGE_CAR_STR = "change_car"; // comando base
const std::string UPGRADE_CAR_STR = "upgrade_car"; // comando para mejora de auto

// Cheats opcodes y strings
constexpr uint8_t CHEAT_CMD = 0x50;                         // opcode para cheats
const std::string CHEAT_GOD_MODE_STR = "cheat_god";         // vida infinita toggle
const std::string CHEAT_DIE_STR = "cheat_die";              // morir/perder
const std::string CHEAT_SKIP_LAP_STR = "cheat_skip_lap";    // completar vuelta actual
const std::string CHEAT_FULL_UPGRADE_STR = "cheat_full_upgrade"; // todas las mejoras al max

// Tipos de cheat (para el payload del opcode CHEAT_CMD)
enum class CheatType : uint8_t {
    GOD_MODE = 0,
    DIE = 1,
    SKIP_LAP = 2,
    FULL_UPGRADE = 3
};


const std::string MOVE_UP_PRESSED_STR = "move_up_pressed";         // NOLINT
const std::string MOVE_UP_RELEASED_STR = "move_up_released";       // NOLINT
const std::string MOVE_DOWN_PRESSED_STR = "move_down_pressed";     // NOLINT
const std::string MOVE_DOWN_RELEASED_STR = "move_down_released";   // NOLINT
const std::string MOVE_LEFT_PRESSED_STR = "move_left_pressed";     // NOLINT
const std::string MOVE_LEFT_RELEASED_STR = "move_left_released";   // NOLINT
const std::string MOVE_RIGHT_PRESSED_STR = "move_right_pressed";   // NOLINT
const std::string MOVE_RIGHT_RELEASED_STR = "move_right_released"; // NOLINT

// Lobby commands
const std::string CREATE_GAME_STR = "create_game"; // NOLINT
const std::string JOIN_GAME_STR = "join_game";     // NOLINT
const std::string GET_GAMES_STR = "get_games";     // NOLINT
const std::string START_GAME_STR = "start_game";   // NOLINT
const std::string LEAVE_GAME_STR = "leave_game";   // NOLINT

const int LIBERTY_CITY_MAP_ID = 1;
const int SAN_ANDREAS_MAP_ID = 2;
const int VICE_CITY_MAP_ID = 3;

#define COLLISION_FLOOR 0x0001     // 1
#define COLLISION_BRIDGE 0x0002    // 2
#define COLLISION_UNDER 0x0004     // 4
#define SENSOR_START_BRIDGE 0x0008 // 8
#define SENSOR_END_BRIDGE 0x0010   // 16

#define CAR_GROUND 0x0020 // 32
#define CAR_BRIDGE 0x0040 // 64

#define GREEN_CAR "green_car"
#define RED_SQUARED_CAR "red_squared_car"
#define RED_SPORTS_CAR "red_sports_car"
#define LIGHT_BLUE_CAR "light_blue_car"
#define RED_JEEP_CAR "red_jeep_car"
#define PURPLE_TRUCK "purple_truck"
#define LIMOUSINE_CAR "limousine_car"
#define DEFAULTS "defaults"

constexpr int CAR_TYPES_COUNT = 7;

inline const char *const CAR_TYPES[] = {
    GREEN_CAR,
    RED_SQUARED_CAR,
    RED_SPORTS_CAR,
    LIGHT_BLUE_CAR,
    RED_JEEP_CAR,
    PURPLE_TRUCK,
    LIMOUSINE_CAR};

enum class CarUpgrade {
    ACCELERATION_BOOST = 0,
    SPEED_BOOST = 1,
    HANDLING_IMPROVEMENT = 2,
    DURABILITY_ENHANCEMENT = 3
};

// Constantes para parseo del JSON del mapa
#define LAYERS_STR "layers"
#define TYPE_STR "type"
#define OBJECTGROUP_STR "objectgroup"
#define NAME_STR "name"
#define OBJECTS_STR "objects"
#define X_STR "x"
#define Y_STR "y"
#define WIDTH_STR "width"
#define HEIGHT_STR "height"
#define POLYGON_STR "polygon"
#define CONNECTIONS_STR "connections"
#define CHECKPOINTS_STR "checkpoints"
#define WAYPOINTS_STR "waypoints"
#define PARKED_CARS_STR "parked_cars"
#define HORIZONTAL_STR "value"
#define LAYER_COLLISIONS_STR "Collisions"
#define LAYER_COLLISIONS_BRIDGE_STR "Collisions_Bridge"
#define LAYER_END_BRIDGE_STR "End_Bridge"
#define LAYER_COLLISIONS_UNDER_STR "Collisions_under"
#define LAYER_START_BRIDGE_STR "Start_Bridge"

#define SPEED_UPGRADE_MULTIPLIER 1.15f
#define ACCELERATION_UPGRADE_MULTIPLIER 1.15f
#define DURABILITY_UPGRADE_REDUCTION 1.0f
#define HANDLING_UPGRADE_MULTIPLIER 1.15f
#define MAX_UPGRADES_PER_STAT 3

// Campeonato: 3 rondas
constexpr int TOTAL_ROUNDS = 3;

#endif
