#ifndef TICK_PROCESSOR_H
#define TICK_PROCESSOR_H

#include <mutex>
#include <unordered_map>
#include <vector>
#include "../../game_state.h"
#include "../../PlayerData.h"
#include "../state/game_state_manager.h"
#include "../player/player_manager.h"
#include "../npc/npc_manager.h"
#include "../world/world_manager.h"
#include "../broadcast/broadcast_manager.h"
#include "../bridge/bridge_handler.h"
#include "../race/race_manager.h"
#include "../collision/collision_handler.h"

class TickProcessor
{
public:
    TickProcessor(
        std::mutex &players_map_mutex,
        std::unordered_map<int, PlayerData> &players,
        GameStateManager &state_manager,
        PlayerManager &player_manager,
        NPCManager &npc_manager,
        WorldManager &world_manager,
        BroadcastManager &broadcast_manager,
        std::vector<b2Vec2> &checkpoint_centers);

    // Process one tick based on current state
    void process(GameState state, float &acum);

private:
    void process_playing(float &acum);
    void process_lobby();
    void process_starting();

    // Helper for deferred body destruction
    void flush_deferred_operations();

    // Helper to broadcast positions (used by playing and starting)
    void broadcast_positions_update();

    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    GameStateManager &state_manager;
    PlayerManager &player_manager;
    NPCManager &npc_manager;
    WorldManager &world_manager;
    BroadcastManager &broadcast_manager;
    std::vector<b2Vec2> &checkpoint_centers;

    static constexpr float FPS = 1.0f / 60.0f;
    static constexpr int VELOCITY_ITERS = 8;
    static constexpr int COLLISION_ITERS = 3;
};

#endif

