#include "collision_handler.h"
#include "../gameloop_constants.h"
#include <iostream>

int CollisionHandler::find_player_by_body(
    b2Body *body,
    const std::unordered_map<int, PlayerData> &players)
{
    for (const auto &entry : players)
    {
        if (entry.second.body == body)
            return entry.first; // player id
    }
    return -1;
}

float CollisionHandler::calculate_frontal_multiplier(const b2Vec2 &vel_a, const b2Vec2 &vel_b)
{
    float frontal_multiplier = 1.0f;
    
    if (vel_a.Length() > 1.0f && vel_b.Length() > 1.0f)
    {
        // Normalizar y calcular producto punto
        b2Vec2 dir_a = vel_a;
        dir_a.Normalize();
        b2Vec2 dir_b = vel_b;
        dir_b.Normalize();
        float dot = b2Dot(dir_a, dir_b);

        // Si dot < -0.5, significa que los vectores apuntan en direcciones opuestas
        // (ángulo > 120°). Cuanto más negativo, más frontal es el choque.
        if (dot < -0.5f)
        {
            // dot = -1.0 (180°) -> multiplier = 2.5x
            // dot = -0.5 (120°) -> multiplier = 1.5x
            frontal_multiplier = 1.5f + (-dot - 0.5f) * 2.0f;
        }
    }
    
    return frontal_multiplier;
}

bool CollisionHandler::process_player_collision_damage(
    b2Body *player_body,
    b2Body *other_body,
    std::unordered_map<int, PlayerData> &players)
{
    int player_id = find_player_by_body(player_body, players);
    if (player_id == -1)
        return false;

    auto it = players.find(player_id);
    if (it == players.end())
        return false;

    b2Vec2 vel_player = player_body->GetLinearVelocity();
    b2Vec2 vel_other = other_body->GetLinearVelocity();
    b2Vec2 relative_vel = vel_player - vel_other;
    float impact_velocity = relative_vel.Length();

    float frontal_multiplier = calculate_frontal_multiplier(vel_player, vel_other);
    return apply_collision_damage(it->second, impact_velocity, frontal_multiplier);
}

bool CollisionHandler::handle_car_collision(
    b2Fixture *fixture_a,
    b2Fixture *fixture_b,
    std::unordered_map<int, PlayerData> &players,
    GameState game_state)
{
    if (game_state != GameState::PLAYING)
        return false;

    b2Body *body_a = fixture_a->GetBody();
    b2Body *body_b = fixture_b->GetBody();

    uint16 cat_a = fixture_a->GetFilterData().categoryBits;
    uint16 cat_b = fixture_b->GetFilterData().categoryBits;

    // Skipeo si alguno es sensor
    bool a_is_sensor = fixture_a->IsSensor();
    bool b_is_sensor = fixture_b->IsSensor();

    if (a_is_sensor || b_is_sensor)
        return false;

    bool a_is_player = (cat_a == CAR_GROUND || cat_a == CAR_BRIDGE);
    bool b_is_player = (cat_b == CAR_GROUND || cat_b == CAR_BRIDGE);

    // Determinar tipo de colisión para logging
    std::string collision_type_a = "";
    std::string collision_type_b = "";

    if (a_is_player && b_is_player)
    {
        collision_type_a = "PLAYER vs PLAYER";
        collision_type_b = "PLAYER vs PLAYER";
    }
    else if (a_is_player)
    {
        if (cat_b == 0x0001)
            collision_type_a = "PLAYER vs WALL";
        else
            collision_type_a = "PLAYER vs NPC/OBSTACLE";
    }
    else if (b_is_player)
    {
        if (cat_a == 0x0001)
            collision_type_b = "PLAYER vs WALL";
        else
            collision_type_b = "PLAYER vs NPC/OBSTACLE";
    }

    bool any_death = false;

    // Aplicar daño para el jugador A si es un jugador
    if (a_is_player)
    {
        if (process_player_collision_damage(body_a, body_b, players))
            any_death = true;
    }

    // Aplicar daño para el jugador B si es un jugador
    if (b_is_player && (!a_is_player || !b_is_player))
    {
        // Solo log si no es player vs player (evitar doble log)
        if (process_player_collision_damage(body_b, body_a, players))
            any_death = true;
    }
    else if (b_is_player && a_is_player)
    {
        // Player vs Player: procesar sin log adicional
        int player_b_id = find_player_by_body(body_b, players);
        if (player_b_id != -1)
        {
            auto it_b = players.find(player_b_id);
            if (it_b != players.end())
            {
                b2Vec2 vel_a = body_a->GetLinearVelocity();
                b2Vec2 vel_b = body_b->GetLinearVelocity();
                b2Vec2 relative_vel = vel_b - vel_a;
                float impact_velocity = relative_vel.Length();
                float frontal_multiplier = calculate_frontal_multiplier(vel_a, vel_b);

                if (apply_collision_damage(it_b->second, impact_velocity, 
                                         frontal_multiplier))
                    any_death = true;
            }
        }
    }

    return any_death;
}

bool CollisionHandler::apply_collision_damage(
    PlayerData &player_data,
    float impact_velocity,
    float frontal_multiplier)
{
    if (player_data.is_dead){ return false; }
        
    // Usar durability del player (upgradeada)
    float durability = player_data.car.durability;

    // Formula: damage = impact_velocity * multiplier * frontal_multiplier * 0.1
    // (10 m/s) son aprox 10 damage (o 25 si es frontal)
    float damage = impact_velocity * durability * frontal_multiplier * 0.1f;

    // Solo aplico daño si es significativo
    if (damage < MIN_COLLISION_DAMAGE)
        return false;

    player_data.car.hp -= damage;
    player_data.collision_this_frame = true;

    if (player_data.car.hp <= 0.0f)
    {
        disqualify_player(player_data);
        return true;
    }

    return false;
}

void CollisionHandler::disqualify_player(PlayerData &player_data)
{
    // Marcar como muerto y descalificado
    player_data.car.hp = 0.0f;
    player_data.is_dead = true;
    player_data.race_finished = true;
    player_data.disqualified = true;
    player_data.god_mode = false;

    // Asignar tiempo de descalificación: 10 minutos (mismo que timeout de ronda)
    uint32_t dq_ms = ROUND_TIME_LIMIT_MS;
    int round_idx = player_data.rounds_completed;
    if (round_idx >= 0 && round_idx < TOTAL_ROUNDS)
    {
        // Preservar penalizaciones existentes y sumar la descalificación
        uint32_t existing_time = player_data.round_times_ms[round_idx];
        player_data.round_times_ms[round_idx] = existing_time + dq_ms;
    }

    player_data.rounds_completed = std::min(player_data.rounds_completed + 1, TOTAL_ROUNDS);
    player_data.total_time_ms += dq_ms;

    // Marcar el body para destrucción
    player_data.mark_body_for_removal = true;
    player_data.collision_this_frame = true;
}
