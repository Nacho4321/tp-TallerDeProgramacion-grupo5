#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H
#include "Event.h"
#include <box2d/box2d.h>
struct CarInfo
{
    std::string car_name;
    float speed;
    float acceleration;
    float hp;
};
struct PlayerData
{
    b2Body *body;
    std::string state;
    CarInfo car;
    Position position;
};
#endif