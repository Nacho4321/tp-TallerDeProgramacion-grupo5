#ifndef GAME_STATE_MANAGER_H
#define GAME_STATE_MANAGER_H

#include <chrono>
#include <atomic>
#include <functional>
#include "../../game_state.h"

class GameStateManager
{
public:
    using TransitionCallback = std::function<void()>;

    GameStateManager();

    // State queries
    GameState get_state() const { return game_state; }
    bool is_joinable() const { return game_state == GameState::LOBBY; }
    bool is_playing() const { return game_state == GameState::PLAYING; }
    bool is_starting() const { return game_state == GameState::STARTING; }

    // State transitions
    void transition_to_lobby();
    void transition_to_starting(int countdown_seconds);
    void transition_to_playing();

    // Called each frame to check if countdown finished
    bool check_and_finish_starting();

    // Callbacks for when transitions happen
    void set_on_playing_callback(TransitionCallback callback) { on_playing_callback = callback; }
    void set_on_starting_callback(TransitionCallback callback) { on_starting_callback = callback; }

    // Reset accumulator flag (for physics reset on game start)
    bool should_reset_accumulator();
    void request_accumulator_reset() { reset_accumulator.store(true); }

    // Race reset flag
    std::atomic<bool> &get_pending_race_reset() { return pending_race_reset; }

    // Round timeout tracking
    std::chrono::steady_clock::time_point &get_round_start_time() { return round_start_time; }
    bool &get_round_timeout_checked() { return round_timeout_checked; }

private:
    GameState game_state;
    
    // Starting countdown
    std::chrono::steady_clock::time_point starting_deadline{};
    bool starting_active{false};
    
    // Physics accumulator reset
    std::atomic<bool> reset_accumulator{false};
    
    // Race reset flag
    std::atomic<bool> pending_race_reset{false};
    
    // Round timeout
    std::chrono::steady_clock::time_point round_start_time{};
    bool round_timeout_checked{false};

    // Callbacks
    TransitionCallback on_playing_callback;
    TransitionCallback on_starting_callback;
};

#endif 
