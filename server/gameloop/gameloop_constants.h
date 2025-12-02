#ifndef GAMELOOP_CONSTANTS_H
#define GAMELOOP_CONSTANTS_H

#include <cstdint>

constexpr float SCALE = 32.0f;

// Parametros de simulación física
constexpr float FPS = 1.0f / 60.0f;
constexpr int VELOCITY_ITERS = 8;
constexpr int COLLISION_ITERS = 3;

// Collision Categories (Box2D filter bits)
#define COLLISION_FLOOR 0x0001
#define COLLISION_BRIDGE 0x0002
#define COLLISION_UNDER 0x0004
#define SENSOR_START_BRIDGE 0x0008
#define SENSOR_END_BRIDGE 0x0010

#define CAR_GROUND 0x0020
#define CAR_BRIDGE 0x0040

// Constantes de NPC
static constexpr float NPC_DIRECTION_THRESHOLD = 0.05f;
static constexpr float NPC_ARRIVAL_THRESHOLD_M = 0.5f;
static constexpr float MIN_DISTANCE_FROM_PARKED_M = 1.0f;
static constexpr float MIN_DISTANCE_FROM_SPAWN_M = 5.0f;

// Constantes de física
static constexpr float RIGHT_VECTOR_X = 1.0f;
static constexpr float RIGHT_VECTOR_Y = 0.0f;
static constexpr float FORWARD_VECTOR_X = 0.0f;
static constexpr float FORWARD_VECTOR_Y = 1.0f;

// Constantes de player manager
static constexpr int CHECKPOINT_LOOKAHEAD = 3;
static constexpr const char *FULL_LOBBY_MSG = "can't join lobby, maximum players reached";

// Constantes de carrera y campeonato

// Campeonato: 3 rondas
constexpr int TOTAL_ROUNDS = 3;

// Tiempo límite de ronda: 10 minutos en milisegundos
constexpr int ROUND_TIME_LIMIT_MS = 10 * 60 * 1000;

// Umbral de daño por colisión
constexpr float MIN_COLLISION_DAMAGE = 0.5f;

#endif
