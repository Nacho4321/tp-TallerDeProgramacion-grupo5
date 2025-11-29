#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H
#include "event.h"
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
    // Si el jugador murió (HP <= 0)
    bool is_dead = false;
    // Flags para colisiones y body management
    bool collision_this_frame = false;
    bool mark_body_for_removal = false;

    // --- Timing de carreras y campeonato ---
    // Tiempos por ronda (en ms), máx 3 rondas
    std::vector<uint32_t> round_times_ms;
    // Tiempo total acumulado del campeonato (en ms)
    uint32_t total_time_ms = 0;
    // Cantidad de rondas completadas (0..3)
    int rounds_completed = 0;
    // Flag de descalificación de la ronda actual (muerte cuenta como 10 min)
    bool disqualified = false;
};
#endif