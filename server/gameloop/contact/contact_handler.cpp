#include "contact_handler.h"

ContactHandler::ContactHandler(
    std::mutex &players_map_mutex,
    std::unordered_map<int, PlayerData> &players,
    std::unordered_map<b2Fixture *, int> &checkpoint_fixtures,
    std::vector<b2Vec2> &checkpoint_centers,
    std::atomic<bool> &pending_race_reset,
    std::function<GameState()> get_state)
    : players_map_mutex(players_map_mutex),
      players(players),
      checkpoint_fixtures(checkpoint_fixtures),
      checkpoint_centers(checkpoint_centers),
      pending_race_reset(pending_race_reset),
      get_state(get_state)
{
}

void ContactHandler::handle_begin_contact(b2Fixture *fixture_a, b2Fixture *fixture_b)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);

    // Checkeo checkpoint
    process_pair(fixture_a, fixture_b);
    process_pair(fixture_b, fixture_a);

    // Checkeo colisiones entre autos
    bool any_death = CollisionHandler::handle_car_collision(fixture_a, fixture_b, players, get_state());
    if (any_death)
    {
        RaceManager::check_race_completion(players, pending_race_reset);
    }
}

void ContactHandler::process_pair(b2Fixture *maybePlayerFix, b2Fixture *maybeCheckpointFix)
{
    int player_id, checkpoint_index;
    if (!CheckpointHandler::is_valid_checkpoint_collision(
            maybePlayerFix, maybeCheckpointFix, players, checkpoint_fixtures,
            player_id, checkpoint_index))
        return;

    PlayerData &player_data = players[player_id];
    int total = static_cast<int>(checkpoint_centers.size());
    bool completed_lap = CheckpointHandler::handle_checkpoint_reached(
        player_data, player_id, checkpoint_index, total);

    if (completed_lap)
    {
        RaceManager::complete_player_race(player_data, pending_race_reset, players);
    }
}

