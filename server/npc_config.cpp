#include "npc_config.h"
#include "../common/constants.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#define NPC_NAME "npc"
#define MAX_MOVING_NPCS_STR "max_moving"
#define MAX_PARKED_NPCS_STR "max_parked"
#define SPEED_PX_S_STR "speed_px_s"

NPCConfig::NPCConfig() : max_moving(MAX_MOVING_NPCS), max_parked(MAX_PARKED_NPCS), speed_px_s(NPC_SPEED_PX_S), config_path("config/npc.yaml") {}

NPCConfig &NPCConfig::getInstance()
{
    static NPCConfig instance;
    return instance;
}

bool NPCConfig::loadFromFile(const std::string &path)
{
    config_path = path;
    try
    {
        YAML::Node root = YAML::LoadFile(path);
        YAML::Node npc = root[NPC_NAME];
        max_moving = npc[MAX_MOVING_NPCS_STR].as<int>();
        max_parked = npc[MAX_PARKED_NPCS_STR].as<int>();
        speed_px_s = npc[SPEED_PX_S_STR].as<float>();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[NPCConfig] Error loading NPC config: " << e.what() << std::endl;
        return false;
    }
}
