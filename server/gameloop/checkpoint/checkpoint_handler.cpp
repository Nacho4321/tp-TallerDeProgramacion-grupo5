#include "checkpoint_handler.h"
#include "../gameloop_constants.h"
#include "../../../common/constants.h"
#include <iostream>

void CheckpointHandler::setup_checkpoints_from_file(
    const std::string &json_path,
    b2World &world,
    MapLayout &map_layout,
    std::vector<b2Vec2> &checkpoint_centers,
    std::unordered_map<b2Fixture *, int> &checkpoint_fixtures)
{
    std::vector<b2Vec2> checkpoints;
    map_layout.extract_checkpoints(json_path, checkpoints);

    if (checkpoints.empty())
        return;

    checkpoint_centers = checkpoints;
    for (size_t i = 0; i < checkpoint_centers.size(); ++i)
    {
        b2BodyDef bd;
        bd.type = b2_staticBody;
        bd.position = checkpoint_centers[i];
        b2Body *checkpoint_body = world.CreateBody(&bd);

        b2CircleShape shape;
        shape.m_p.Set(0.0f, 0.0f);
        shape.m_radius = CHECKPOINT_RADIUS_PX / SCALE;

        b2FixtureDef fd;
        fd.shape = &shape;
        fd.isSensor = true;

        b2Fixture *fixture = checkpoint_body->CreateFixture(&fd);
        checkpoint_fixtures[fixture] = static_cast<int>(i);
    }

    std::cout << "[CheckpointHandler] Created " << checkpoint_centers.size()
              << " checkpoint sensors (from " << json_path << ")." << std::endl;
}

void CheckpointHandler::load_round_checkpoints(
    int current_round,
    const std::array<std::string, 3> &checkpoint_sets,
    b2World &world,
    MapLayout &map_layout,
    std::vector<b2Vec2> &checkpoint_centers,
    std::unordered_map<b2Fixture *, int> &checkpoint_fixtures)
{
    // Limpiar checkpoints previos y fixtures
    for (auto &pair : checkpoint_fixtures)
    {
        b2Body *body = pair.first->GetBody();
        if (body)
        {
            world.DestroyBody(body);
        }
    }
    checkpoint_fixtures.clear();
    checkpoint_centers.clear();

    std::string json_path = checkpoint_sets[current_round];
    setup_checkpoints_from_file(json_path, world, map_layout, checkpoint_centers, checkpoint_fixtures);
    std::cout << "[CheckpointHandler] Round " << current_round + 1 << " loaded checkpoints from: " << json_path << std::endl;
}

int CheckpointHandler::find_player_by_body(
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

bool CheckpointHandler::is_valid_checkpoint_collision(
    b2Fixture *player_fixture,
    b2Fixture *checkpoint_fixture,
    const std::unordered_map<int, PlayerData> &players,
    const std::unordered_map<b2Fixture *, int> &checkpoint_fixtures,
    int &out_player_id,
    int &out_checkpoint_index)
{
    if (!player_fixture || !checkpoint_fixture)
        return false;

    b2Body *player_body = player_fixture->GetBody();
    out_player_id = find_player_by_body(player_body, players);
    if (out_player_id < 0)
        return false;

    auto it = checkpoint_fixtures.find(checkpoint_fixture);
    if (it == checkpoint_fixtures.end())
        return false;

    out_checkpoint_index = it->second;
    return true;
}

bool CheckpointHandler::handle_checkpoint_reached(
    PlayerData &player_data,
    int player_id,
    int checkpoint_index,
    int total_checkpoints)
{
    if (player_data.race_finished)
        return false;

    if (player_data.next_checkpoint != checkpoint_index)
        return false;

    int new_next = player_data.next_checkpoint + 1;

    if (total_checkpoints > 0 && new_next >= total_checkpoints)
    {
        // Jugador completó la vuelta
        return true;
    }
    else
    {
        player_data.next_checkpoint = new_next;

        std::cout << "[CheckpointHandler] Player " << player_id << " passed checkpoint "
                  << checkpoint_index << " next=" << player_data.next_checkpoint << std::endl;
        return false;
    }
}
