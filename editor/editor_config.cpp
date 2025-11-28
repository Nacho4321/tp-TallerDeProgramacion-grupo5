#include "editor_config.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <cstdlib>

EditorConfig& EditorConfig::getInstance() {
    static EditorConfig instance;
    return instance;
}

std::string EditorConfig::findConfigFile() const {
    std::vector<std::string> possiblePaths = {
        "config/editor.yaml",         
        "../config/editor.yaml",      
        "../../config/editor.yaml"
    };
    
    for (const auto& path : possiblePaths) {
        std::ifstream file(path);
        if (file.good()) {
            file.close();
            return path;
        }
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
            mapImagePath = editor["map_image"].as<std::string>();
        } else {
            std::cerr << "[EditorConfig] 'map_image' no se encuentra en el YAML" << std::endl;
            return false;
        }
        
        if (editor["checkpoints_file"]) {
            checkpointsFilePath = editor["checkpoints_file"].as<std::string>();
        } else {
            std::cerr << "[EditorConfig] 'checkpoints_file' no se encuentra en el YAML" << std::endl;
            return false;
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

std::string EditorConfig::getCheckpointsFilePath() const {
    return checkpointsFilePath;
}
