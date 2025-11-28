#ifndef PLAYERDATA_H
#define PLAYERDATA_H
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
    // Flag para indicar si el jugador colisionó este frame (para animación de explosión)
    bool collision_this_frame = false;
    // Respawn system
    bool waiting_to_respawn = false;
    std::chrono::steady_clock::time_point death_time;
    Position last_checkpoint_position;  // Position of the last checkpoint passed
    bool has_passed_checkpoint = false; // True if player has passed at least one checkpoint
    // When the player dies, we can't destroy the body inside Box2D callbacks.
    // Use this flag to defer body destruction to a safe moment (outside world lock).
    bool mark_body_for_removal = false;
};
#endif