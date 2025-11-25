#ifndef NPC_CONFIG_H
#define NPC_CONFIG_H

#include <string>

class NPCConfig {
private:
    int max_moving;
    int max_parked;
    float speed_px_s;
    std::string config_path;
    NPCConfig();
public:
    static NPCConfig& getInstance();
    NPCConfig(const NPCConfig&) = delete;
    NPCConfig& operator=(const NPCConfig&) = delete;

    bool loadFromFile(const std::string& path);

    int getMaxMoving() const { return max_moving; }
    int getMaxParked() const { return max_parked; }
    float getSpeedPxS() const { return speed_px_s; }
};

#endif // NPC_CONFIG_H
