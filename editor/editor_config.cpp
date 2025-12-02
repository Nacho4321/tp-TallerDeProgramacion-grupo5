#include "editor_config.h"
#include "install_paths.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <cstdlib>

EditorConfig& EditorConfig::getInstance() {
    static EditorConfig instance;
    return instance;
}

std::string EditorConfig::findConfigFile() const {
    std::string configPath = std::string(CONFIG_DIR) + "/editor.yaml";

    std::ifstream file(configPath);
    if (file.good()) {
        file.close();
        return configPath;
    }

    return "";
}

bool EditorConfig::loadFromFile(const std::string& configPath) {
    try {
        std::string actualPath = configPath.empty() ? findConfigFile() : configPath;

        if (actualPath.empty()) {
            std::cerr << "[EditorConfig] No se encontró el archivo de configuración" << std::endl;
            return false;
        }
        
        YAML::Node config = YAML::LoadFile(actualPath);
        
        if (!config["editor"]) {
            std::cerr << "[EditorConfig] 'editor' no se encuentra en el YAML" << std::endl;
            return false;
        }
        
        YAML::Node editor = config["editor"];
        
        if (editor["map_image"]) {
            mapImagePath = std::string(DATA_DIR) + "/" + editor["map_image"].as<std::string>();
        } else {
            std::cerr << "[EditorConfig] 'map_image' no se encuentra en el YAML" << std::endl;
            return false;
        }

        if (editor["checkpoints_files"]) {
            YAML::Node files = editor["checkpoints_files"];

            if (files["race_1"]) {
                checkpointsPaths[RaceId::Race1] = std::string(DATA_DIR) + "/" + files["race_1"].as<std::string>();
            }
            if (files["race_2"]) {
                checkpointsPaths[RaceId::Race2] = std::string(DATA_DIR) + "/" + files["race_2"].as<std::string>();
            }
            if (files["race_3"]) {
                checkpointsPaths[RaceId::Race3] = std::string(DATA_DIR) + "/" + files["race_3"].as<std::string>();
            }

            if (checkpointsPaths.empty()) {
                std::cerr << "[EditorConfig] No se encontraron rutas de checkpoints" << std::endl;
                return false;
            }
        } else {
            if (editor["checkpoints_file"]) {
                checkpointsPaths[RaceId::Race1] = std::string(DATA_DIR) + "/" + editor["checkpoints_file"].as<std::string>();
            } else {
                std::cerr << "[EditorConfig] 'checkpoints_files' no se encuentra en el YAML" << std::endl;
                return false;
            }
        }

        if (editor["spawn_points_file"]) {
            spawnPointsPath = std::string(DATA_DIR) + "/" + editor["spawn_points_file"].as<std::string>();
        }
        
        return true;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "[EditorConfig] Error al parsear YAML: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[EditorConfig] Error: " << e.what() << std::endl;
        return false;
    }
}

std::string EditorConfig::getMapImagePath() const {
    return mapImagePath;
}

std::string EditorConfig::getCheckpointsFilePath(RaceId race) const {
    auto it = checkpointsPaths.find(race);
    if (it != checkpointsPaths.end()) {
        return it->second;
    }
    return "";
}

std::map<RaceId, std::string> EditorConfig::getAllCheckpointsPaths() const {
    return checkpointsPaths;
}

std::string EditorConfig::getSpawnPointsFilePath() const {
    return spawnPointsPath;
}

std::string EditorConfig::getRaceName(RaceId race) {
    switch (race) {
        case RaceId::Race1: return "Race 1";
        case RaceId::Race2: return "Race 2";
        case RaceId::Race3: return "Race 3";
        default: return "Unknown";
    }
}
