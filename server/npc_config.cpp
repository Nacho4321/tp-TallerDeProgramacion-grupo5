#include "npc_config.h"
#include "../common/constants.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

NPCConfig::NPCConfig() : max_moving(MAX_MOVING_NPCS), max_parked(MAX_PARKED_NPCS), speed_px_s(NPC_SPEED_PX_S), config_path("config/npc.yaml") {}

NPCConfig& NPCConfig::getInstance() {
    static NPCConfig instance;
    return instance;
}

bool NPCConfig::loadFromFile(const std::string& path) {
    config_path = path;
    try {
        YAML::Node root = YAML::LoadFile(path);
        if (!root["npc"]) {
            std::cerr << "[NPCConfig] Missing 'npc' section in " << path << std::endl;
            return false; // keep defaults
        }
        YAML::Node npc = root["npc"];
        if (npc["max_moving"]) max_moving = npc["max_moving"].as<int>();
        if (npc["max_parked"]) max_parked = npc["max_parked"].as<int>();
        if (npc["speed_px_s"]) speed_px_s = npc["speed_px_s"].as<float>();
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "[NPCConfig] YAML error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[NPCConfig] Error loading NPC config: " << e.what() << std::endl;
        return false;
    }
}
