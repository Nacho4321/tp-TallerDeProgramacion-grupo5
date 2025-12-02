#include "game_state_manager.h"
#include <iostream>

GameStateManager::GameStateManager()
    : game_state(GameState::LOBBY),
      starting_active(false),
      reset_accumulator(false),
      pending_race_reset(false),
      round_timeout_checked(false)
{
}

void GameStateManager::transition_to_lobby()
{
    game_state = GameState::LOBBY;
    pending_race_reset.store(false);
}

void GameStateManager::transition_to_starting(int countdown_seconds)
{
    starting_active = true;
    game_state = GameState::STARTING;
    starting_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(countdown_seconds);

    if (on_starting_callback)
    {
        on_starting_callback();
    }
}

void GameStateManager::transition_to_playing()
{
    game_state = GameState::PLAYING;
    reset_accumulator.store(true);

    // Iniciar contador de 10 minutos para la ronda
    round_start_time = std::chrono::steady_clock::now();
    round_timeout_checked = false;

    if (on_playing_callback)
    {
        on_playing_callback();
    }
}

bool GameStateManager::check_and_finish_starting()
{
    if (!starting_active)
    {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    if (now >= starting_deadline)
    {
        starting_active = false;
        transition_to_playing();
        return true;
    }
    return false;
}

bool GameStateManager::should_reset_accumulator()
{
    bool expected = true;
    return reset_accumulator.compare_exchange_strong(expected, false);
}
