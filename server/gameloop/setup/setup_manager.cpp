#include "setup_manager.h"
#include "../../npc_config.h"
#include "../gameloop_constants.h"
#include "../../../common/constants.h"

SetupManager::SetupManager(
    uint8_t map_id,
    MapLayout &map_layout,
    WorldManager &world_manager,
    NPCManager &npc_manager,
    std::array<std::string, 3> &checkpoint_sets,
    std::vector<MapLayout::SpawnPointData> &spawn_points,
    std::unordered_map<b2Fixture *, int> &checkpoint_fixtures,
    std::vector<b2Vec2> &checkpoint_centers)
    : map_id(map_id),
      map_layout(map_layout),
      world_manager(world_manager),
      npc_manager(npc_manager),
      checkpoint_sets(checkpoint_sets),
      spawn_points(spawn_points),
      checkpoint_fixtures(checkpoint_fixtures),
      checkpoint_centers(checkpoint_centers)
{
}

void SetupManager::setup_world(int current_round)
{
    std::vector<MapLayout::ParkedCarData> parked_data;
    std::vector<MapLayout::WaypointData> street_waypoints;
    
    setup_map_layout();
    load_checkpoints(current_round);
    setup_npc_config();

    uint8_t safe_map_id = (map_id < MAP_COUNT) ? map_id : 0;
    map_layout.extract_map_npc_data(MAP_JSON_PATHS[safe_map_id], street_waypoints, parked_data);
    
    if (!parked_data.empty() || !street_waypoints.empty())
    {
        npc_manager.init(parked_data, street_waypoints, spawn_points);
    }
}

void SetupManager::setup_npc_config()
{
    auto &npc_cfg = NPCConfig::getInstance();
    npc_cfg.loadFromFile("config/npc.yaml");
}

void SetupManager::setup_map_layout()
{
    uint8_t safe_map_id = (map_id < MAP_COUNT) ? map_id : 0;
    map_layout.create_map_layout(MAP_JSON_PATHS[safe_map_id]);
}

void SetupManager::load_checkpoints(int current_round)
{
    CheckpointHandler::load_round_checkpoints(
        current_round,
        checkpoint_sets,
        world_manager.get_world(),
        map_layout,
        checkpoint_centers,
        checkpoint_fixtures);
}
