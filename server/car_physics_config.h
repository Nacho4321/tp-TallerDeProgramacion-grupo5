#ifndef CAR_PHYSICS_CONFIG_H
#define CAR_PHYSICS_CONFIG_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "../common/constants.h"

struct CarPhysics
{
    float torque;
    float angular_friction;
    float angular_damping;

    float max_speed;
    float max_acceleration;
    float backward_speed_multiplier;
    float speed_controller_gain;

    float max_lateral_impulse;
    float forward_drag_coefficient;
    float linear_damping;

    float density;
    float friction;
    float restitution;
    float width;
    float height;
    float center_offset_y; // Offset vertical del centro de colisión en píxeles

    float max_hp;
    float collision_damage_multiplier;
};

class CarPhysicsConfig
{
private:
    CarPhysics defaults;
    std::unordered_map<std::string, CarPhysics> car_configs;
    std::string config_path;

    CarPhysicsConfig();

    void loadCarPhysicsFromYAML(CarPhysics &physics, const void *node);

public:
    static CarPhysicsConfig &getInstance();

    CarPhysicsConfig(const CarPhysicsConfig &) = delete;
    CarPhysicsConfig &operator=(const CarPhysicsConfig &) = delete;

    bool loadFromFile(const std::string &path);

    // reload config (not used yet)
    bool reload();

    const CarPhysics &getCarPhysics(const std::string &car_name) const;

    bool hasCarType(const std::string &car_name) const;

    std::vector<std::string> getAvailableCarTypes() const;
};

#endif
