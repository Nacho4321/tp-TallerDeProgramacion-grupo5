#ifndef EDITOR_CONFIG_H
#define EDITOR_CONFIG_H

#include <string>
#include <map>

enum class RaceId {
    Race1 = 1,
    Race2 = 2,
    Race3 = 3
};

class EditorConfig {
public:
    static EditorConfig& getInstance();
    
    bool loadFromFile(const std::string& configPath);
    
    std::string getMapImagePath() const;
    
    std::string getCheckpointsFilePath(RaceId race) const;
    std::map<RaceId, std::string> getAllCheckpointsPaths() const;
    static std::string getRaceName(RaceId race);

private:
    EditorConfig() = default;
    ~EditorConfig() = default;
    
    EditorConfig(const EditorConfig&) = delete;
    EditorConfig& operator=(const EditorConfig&) = delete;
    
    std::string mapImagePath;
    std::map<RaceId, std::string> checkpointsPaths;
    
    std::string findConfigFile() const;
};

#endif 
