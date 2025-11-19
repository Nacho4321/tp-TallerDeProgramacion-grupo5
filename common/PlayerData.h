#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H
#include "Event.h"
#include <box2d/b2_body.h>
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
    // Indice del próximo checkpoint que el jugador debe pasar (0-based)
    int next_checkpoint = 0;
    // Cuántas vueltas completas (listas de checkpoints) el jugador ha completado
    int laps_completed = 0;
};
#endif