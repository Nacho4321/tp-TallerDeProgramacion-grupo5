#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H
#include "Event.h"
#include <box2d/b2_body.h>
#include <chrono>
struct CarInfo
{
    std::string car_name;
    float speed;
    float acceleration;
    float hp;
};
struct PlayerData
{
    bool on_bridge;
    b2Body *body;
    std::string state;
    CarInfo car;
    Position position;
    // Indice del próximo checkpoint que el jugador debe pasar (empieza en 0)
    int next_checkpoint = 0;
    // Cuántas vueltas completas (listas de checkpoints) el jugador ha completado
    int laps_completed = 0;
    // Tiempo de inicio de la vuelta actual
    std::chrono::steady_clock::time_point lap_start_time;
    // Si el jugador ya completó la carrera (esperando a que otros terminen)
    bool race_finished = false;
};
#endif