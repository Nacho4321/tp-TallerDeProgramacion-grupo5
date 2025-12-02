#ifndef GAMELOOP_CONSTANTS_H
#define GAMELOOP_CONSTANTS_H

#include <cstdint>

// Physics & World Constants

constexpr float SCALE = 32.0f;

// Physics simulation parameters
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

// Race & Championship Constants

// Championship: 3 rounds
constexpr int TOTAL_ROUNDS = 3;

// Round time limit: 10 minutes in milliseconds
constexpr int ROUND_TIME_LIMIT_MS = 10 * 60 * 1000;

#endif
