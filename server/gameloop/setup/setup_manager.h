#ifndef SETUP_MANAGER_H
#define SETUP_MANAGER_H

#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <box2d/b2_fixture.h>
#include <box2d/b2_math.h>
#include "../../map_layout.h"
#include "../world/world_manager.h"
#include "../npc/npc_manager.h"
#include "../checkpoint/checkpoint_handler.h"

class SetupManager
{
public:
    SetupManager(
        uint8_t map_id,
        MapLayout &map_layout,
        WorldManager &world_manager,
        NPCManager &npc_manager,
        std::array<std::string, 3> &checkpoint_sets,
        std::vector<MapLayout::SpawnPointData> &spawn_points,
        std::unordered_map<b2Fixture *, int> &checkpoint_fixtures,
        std::vector<b2Vec2> &checkpoint_centers);

    // Full world setup (calls all other setup methods)
    void setup_world(int current_round);

    // Individual setup methods
    void setup_npc_config();
    void setup_map_layout();
    void load_checkpoints(int current_round);

private:
    uint8_t map_id;
    MapLayout &map_layout;
    WorldManager &world_manager;
    NPCManager &npc_manager;
    std::array<std::string, 3> &checkpoint_sets;
    std::vector<MapLayout::SpawnPointData> &spawn_points;
    std::unordered_map<b2Fixture *, int> &checkpoint_fixtures;
    std::vector<b2Vec2> &checkpoint_centers;
};

#endif
