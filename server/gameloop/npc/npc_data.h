#ifndef NPC_DATA_H
#define NPC_DATA_H

#include <box2d/b2_body.h>

struct NPCData
{
    b2Body *body{nullptr};
    int npc_id{0};             // id negativo para el broadcast
    int current_waypoint{0};   // último waypoint alcanzado
    int target_waypoint{0};    // próximo waypoint objetivo
    float speed_mps{0.0f};     // velocidad de movimiento en metros/segundo
    bool is_parked{false};     // true = estacionado (cuerpo estático)
    bool is_horizontal{false}; // true = orientado horizontalmente (solo para estacionados)
    bool on_bridge{false};
};

#endif // NPC_DATA_H
