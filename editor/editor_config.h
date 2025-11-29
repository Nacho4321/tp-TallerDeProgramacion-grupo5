#ifndef EDITOR_CONFIG_H
#define EDITOR_CONFIG_H

#include <string>

class EditorConfig {
public:
    static EditorConfig& getInstance();
    
    bool loadFromFile(const std::string& configPath);
    
    std::string getMapImagePath() const;
    std::string getCheckpointsFilePath() const;

private:
    EditorConfig() = default;
    ~EditorConfig() = default;
    
    EditorConfig(const EditorConfig&) = delete;
    EditorConfig& operator=(const EditorConfig&) = delete;
    
    std::string mapImagePath;
    std::string checkpointsFilePath;
    
    std::string findConfigFile() const;
};

#endif 
