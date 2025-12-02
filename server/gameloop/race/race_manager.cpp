#include "race_manager.h"
#include "../gameloop_constants.h"
#include <iostream>

void RaceManager::complete_player_race(
    PlayerData &player_data,
    std::atomic<bool> &pending_race_reset,
    const std::unordered_map<int, PlayerData> &players)
{
    auto lap_end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          lap_end_time - player_data.lap_start_time)
                          .count();

    player_data.race_finished = true;
    player_data.disqualified = false;

    // Guardar el tiempo de la ronda actual
    int round_idx = player_data.rounds_completed;
    uint32_t existing_penalization = 0;

    if (round_idx >= 0 && round_idx < TOTAL_ROUNDS)
    {
        // Si ya hay penalizaciones acumuladas en esta ronda, preservarlas
        existing_penalization = player_data.round_times_ms[round_idx];
        uint32_t new_total_time = static_cast<uint32_t>(elapsed_ms) + existing_penalization;

        player_data.round_times_ms[round_idx] = new_total_time;
    }
    player_data.rounds_completed = std::min(player_data.rounds_completed + 1, TOTAL_ROUNDS);
    player_data.total_time_ms += static_cast<uint32_t>(elapsed_ms);

    player_data.god_mode = true;

    // Verificar si todos terminaron
    check_race_completion(players, pending_race_reset);
}

void RaceManager::check_race_completion(
    const std::unordered_map<int, PlayerData> &players,
    std::atomic<bool> &pending_race_reset)
{
    // Asume que ya estamos bajo players_map_mutex lock
    // IMPORTANTE: No podemos modificar Box2D aca (estamos en callback de contacto)

    int finished_or_dead_count = 0;
    int total_players = static_cast<int>(players.size());

    for (const auto &[id, player_data] : players)
    {
        // Contar jugadores que terminaron la carrera O murieron
        if (player_data.race_finished || player_data.is_dead)
        {
            finished_or_dead_count++;
        }
    }

    // La carrera termina cuando TODOS los jugadores terminaron O murieron
    bool all_done = (finished_or_dead_count == total_players && total_players > 0);

    if (all_done)
    {
        pending_race_reset.store(true);
    }
}

void RaceManager::reset_players_for_race_start(
    std::unordered_map<int, PlayerData> &players,
    CarPhysicsConfig &physics_config)
{
    for (auto &[id, player_data] : players)
    {
        if (!player_data.body)
            continue;

        player_data.body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
        player_data.body->SetAngularVelocity(0.0f);

        // lap_start_time se setea después del countdown
        player_data.next_checkpoint = 0;
        player_data.race_finished = false;
        player_data.is_dead = false;
        player_data.god_mode = false;
        player_data.position.on_bridge = false;
        player_data.position.direction_x = not_horizontal;
        player_data.position.direction_y = not_vertical;

        // Reset HP
        const CarPhysics &car_physics = physics_config.getCarPhysics(player_data.car.car_name);
        player_data.car.hp = car_physics.max_hp;
    }
}

void RaceManager::check_round_timeout(
    std::unordered_map<int, PlayerData> &players,
    GameState game_state,
    bool &round_timeout_checked,
    const std::chrono::steady_clock::time_point &round_start_time,
    std::atomic<bool> &pending_race_reset)
{
    if (game_state != GameState::PLAYING || round_timeout_checked)
        return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - round_start_time).count();

    // Verificar si se cumplieron los 10 minutos
    if (elapsed_ms >= ROUND_TIME_LIMIT_MS)
    {
        // Penalizar a todos los jugadores que NO han terminado la ronda
        for (auto &[id, player_data] : players)
        {
            if (!player_data.race_finished && !player_data.is_dead)
            {
                // Aplicar la penalización de 10 minutos
                uint32_t timeout_penalty = 10u * 60u * 1000u; // 10 minutos en ms
                int round_idx = player_data.rounds_completed;

                if (round_idx >= 0 && round_idx < TOTAL_ROUNDS)
                {
                    uint32_t existing_time = player_data.round_times_ms[round_idx];
                    player_data.round_times_ms[round_idx] = existing_time + timeout_penalty;
                    player_data.total_time_ms = player_data.round_times_ms[0] +
                                                player_data.round_times_ms[1] +
                                                player_data.round_times_ms[2];
                }

                player_data.race_finished = true;
                player_data.god_mode = true;
            }
        }

        pending_race_reset.store(true);
        round_timeout_checked = true;
    }
}

bool RaceManager::should_reset_race(const std::atomic<bool> &pending_race_reset)
{
    return pending_race_reset.load();
}
