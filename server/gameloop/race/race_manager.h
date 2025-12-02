#ifndef RACE_MANAGER_H
#define RACE_MANAGER_H

#include <unordered_map>
#include <chrono>
#include <atomic>
#include "../../PlayerData.h"
#include "../../car_physics_config.h"
#include "../../game_state.h"
#include "../gameloop_constants.h"

class RaceManager
{
public:
    // Completa la carrera de un jugador, guardando su tiempo
    // Retorna true si se debe verificar el fin de la carrera
    static void complete_player_race(
        PlayerData &player_data,
        std::atomic<bool> &pending_race_reset,
        const std::unordered_map<int, PlayerData> &players);

    // Verifica si todos los jugadores terminaron o murieron
    // Si es así, marca pending_race_reset = true
    static void check_race_completion(
        const std::unordered_map<int, PlayerData> &players,
        std::atomic<bool> &pending_race_reset);

    // Resetea el estado de los jugadores para el inicio de una nueva carrera
    // (velocidades, checkpoint, HP, etc.) - NO recrea los bodies
    static void reset_players_for_race_start(
        std::unordered_map<int, PlayerData> &players,
        CarPhysicsConfig &physics_config);

    // Verifica si se cumplió el timeout de la ronda (10 minutos)
    // Penaliza a los jugadores que no terminaron y marca pending_race_reset
    static void check_round_timeout(
        std::unordered_map<int, PlayerData> &players,
        GameState game_state,
        bool &round_timeout_checked,
        const std::chrono::steady_clock::time_point &round_start_time,
        std::atomic<bool> &pending_race_reset);

    // Verifica si se debe resetear la carrera
    static bool should_reset_race(const std::atomic<bool> &pending_race_reset);
};

#endif
