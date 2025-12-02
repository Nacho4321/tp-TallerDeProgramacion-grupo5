#ifndef COLLISION_HANDLER_H
#define COLLISION_HANDLER_H

#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_contact.h>
#include <unordered_map>
#include <string>
#include "../../PlayerData.h"
#include "../../car_physics_config.h"
#include "../../game_state.h"

class CollisionHandler
{
public:
    // Encuentra el ID del jugador por su body de Box2D
    static int find_player_by_body(
        b2Body *body,
        const std::unordered_map<int, PlayerData> &players);

    // Maneja colisiones entre autos (jugador vs jugador, jugador vs NPC, jugador vs pared)
    // Retorna true si algún jugador murió por esta colisión
    static bool handle_car_collision(
        b2Fixture *fixture_a,
        b2Fixture *fixture_b,
        std::unordered_map<int, PlayerData> &players,
        GameState game_state);

    // Aplica daño por colisión a un jugador
    // Retorna true si el jugador murió por esta colisión
    static bool apply_collision_damage(
        PlayerData &player_data,
        float impact_velocity,
        float frontal_multiplier = 1.0f);

    // Descalifica a un jugador (muerte por daño)
    static void disqualify_player(PlayerData &player_data);

private:
    // Calcula el multiplicador de daño frontal basado en las velocidades
    static float calculate_frontal_multiplier(const b2Vec2 &vel_a, const b2Vec2 &vel_b);

    // Procesa el daño para un jugador específico en una colisión
    // Retorna true si el jugador murió
    static bool process_player_collision_damage(
        b2Body *player_body,
        b2Body *other_body,
        std::unordered_map<int, PlayerData> &players);
};

#endif
