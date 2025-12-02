#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H
#include "event.h"
#include <box2d/b2_body.h>
#include <chrono>
#include <vector>

struct UpgradeLevels
{
    uint8_t speed = 0;
    uint8_t acceleration = 0;
    uint8_t handling = 0;
    uint8_t durability = 0;
};

struct CarInfo
{
    std::string car_name;
    float speed;
    float acceleration;
    float hp;
    float durability;
    float handling;
};

struct PlayerData
{
    b2Body *body;
    std::string state;
    CarInfo car;
    UpgradeLevels upgrades;
    Position position;

    int next_checkpoint = 0;
    std::chrono::steady_clock::time_point lap_start_time;

    bool race_finished = false;
    bool is_dead = false;
    // Flags para colisiones y body management
    bool collision_this_frame = false;
    bool mark_body_for_removal = false;

    std::vector<uint32_t> round_times_ms{0, 0, 0};
    uint32_t total_time_ms = 0;
    int rounds_completed = 0;
    bool disqualified = false;
    bool is_stopping = false;
    bool god_mode = false;
    bool pending_disqualification = false;
    bool pending_race_complete = false;
};
#endif