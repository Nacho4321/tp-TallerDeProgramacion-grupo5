#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H
#include "Event.h"
struct CarInfo
{
    std::string car_name;
    float speed;
    float acceleration;
    float hp;
};
struct PlayerData
{
    std::string state;
    CarInfo car;
    Position position;
};
#endif