#include "tick_processor.h"
#include <iostream>

TickProcessor::TickProcessor(
    std::mutex &players_map_mutex,
    std::unordered_map<int, PlayerData> &players,
    GameStateManager &state_manager,
    PlayerManager &player_manager,
    NPCManager &npc_manager,
    WorldManager &world_manager,
    BroadcastManager &broadcast_manager,
    std::vector<b2Vec2> &checkpoint_centers)
    : players_map_mutex(players_map_mutex),
      players(players),
      state_manager(state_manager),
      player_manager(player_manager),
      npc_manager(npc_manager),
      world_manager(world_manager),
      broadcast_manager(broadcast_manager),
      checkpoint_centers(checkpoint_centers)
{
}

void TickProcessor::process(GameState state, float &acum)
{
    switch (state)
    {
    case GameState::PLAYING:
        process_playing(acum);
        break;
    case GameState::LOBBY:
        process_lobby();
        break;
    case GameState::STARTING:
        process_starting();
        break;
    }
}

void TickProcessor::process_playing(float &acum)
{
    if (players.empty())
        return;

    if (state_manager.should_reset_accumulator())
    {
        acum = 0.0f;
        std::cout << "[TickProcessor] Physics accumulator reset on start." << std::endl;
    }

    RaceManager::check_round_timeout(
        players,
        state_manager.get_state(),
        state_manager.get_round_timeout_checked(),
        state_manager.get_round_start_time(),
        state_manager.get_pending_race_reset());

    // Reset collision flags at the start of each frame
    for (auto &entry : players)
    {
        entry.second.collision_this_frame = false;
    }

    npc_manager.update();
    player_manager.update_body_positions();

    while (acum >= FPS)
    {
        world_manager.step(FPS, VELOCITY_ITERS, COLLISION_ITERS);
        acum -= FPS;
    }

    flush_deferred_operations();
    broadcast_positions_update();
}

void TickProcessor::process_lobby()
{
    if (players.empty())
        return;

    // Reset collision flags at the start of each frame
    for (auto &entry : players)
    {
        entry.second.collision_this_frame = false;
    }
}

void TickProcessor::process_starting()
{
    // Reset collision flags at the start of each frame
    for (auto &entry : players)
    {
        entry.second.collision_this_frame = false;
    }

    broadcast_positions_update();
    state_manager.check_and_finish_starting();
}

void TickProcessor::flush_deferred_operations()
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    for (auto &[id, player_data] : players)
    {
        // Procesar cheat de completar ronda pendiente
        if (player_data.pending_race_complete && !player_data.race_finished)
        {
            RaceManager::complete_player_race(player_data, state_manager.get_pending_race_reset(), players);
            player_data.pending_race_complete = false;
        }

        // Procesar cheat de descalificación pendiente
        if (player_data.pending_disqualification && !player_data.is_dead)
        {
            CollisionHandler::disqualify_player(player_data);
            RaceManager::check_race_completion(players, state_manager.get_pending_race_reset());
            player_data.pending_disqualification = false;
        }

        if (player_data.mark_body_for_removal && player_data.body)
        {
            if (world_manager.is_locked())
            {
                std::cout << "[TickProcessor] World locked during flush, postergando destroy para player " << id << std::endl;
                continue;
            }
            world_manager.safe_destroy_body(player_data.body);
            player_data.mark_body_for_removal = false;
            std::cout << "[TickProcessor] Destroyed body for player " << id << " (flush)." << std::endl;
        }
    }
}

void TickProcessor::broadcast_positions_update()
{
    std::vector<PlayerPositionUpdate> broadcast;
    player_manager.update_player_positions(broadcast, checkpoint_centers);

    // Actualizar estado del bridge para NPCs
    for (auto &npc : npc_manager.get_npcs())
    {
        BridgeHandler::update_bridge_state(npc);
    }
    npc_manager.add_to_broadcast(broadcast);

    ServerMessage msg;
    msg.opcode = UPDATE_POSITIONS;
    msg.positions = broadcast;
    broadcast_manager.broadcast(msg);
}

