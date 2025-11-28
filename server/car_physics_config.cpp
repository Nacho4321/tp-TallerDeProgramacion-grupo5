#include "car_physics_config.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <vector>

CarPhysicsConfig::CarPhysicsConfig() : config_path("config/car_physics.yaml")
{
    // Inicializar defaults con valores seguros
    defaults.center_offset_y = 0.0f;
}

CarPhysicsConfig &CarPhysicsConfig::getInstance()
{
    static CarPhysicsConfig instance;
    return instance;
}

bool CarPhysicsConfig::loadFromFile(const std::string &path)
{
    config_path = path;

    try
    {
        YAML::Node config = YAML::LoadFile(path);

        if (config["defaults"])
        {
            YAML::Node default_config = config["defaults"];
            loadCarPhysicsFromYAML(defaults, &default_config);
        }

        // Load individual car configurations
        if (config["cars"])
        {
            const YAML::Node &cars = config["cars"];

            for (YAML::const_iterator it = cars.begin(); it != cars.end(); ++it)
            {
                std::string car_name = it->first.as<std::string>();

                // Start with defaults, then override with car-specific values
                CarPhysics car_physics = defaults;
                loadCarPhysicsFromYAML(car_physics, &it->second);

                car_configs[car_name] = car_physics;
                std::cout << "[CarPhysicsConfig] Loaded config for car type: " << car_name << std::endl;
            }
        }

        std::cout << "[CarPhysicsConfig] Successfully loaded " << car_configs.size()
                  << " car configurations from " << path << std::endl;
        return true;
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << "[CarPhysicsConfig] YAML error: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CarPhysicsConfig] Error loading config: " << e.what() << std::endl;
        return false;
    }
}

bool CarPhysicsConfig::reload()
{
    std::cout << "[CarPhysicsConfig] Reloading config from " << config_path << std::endl;
    car_configs.clear();
    return loadFromFile(config_path);
}

const CarPhysics &CarPhysicsConfig::getCarPhysics(const std::string &car_name) const
{
    std::string effective = car_name;

    auto it = car_configs.find(effective);
    if (it != car_configs.end())
    {
        return it->second;
    }

    it = car_configs.find(DEFAULTS);
    return it->second;
}

bool CarPhysicsConfig::hasCarType(const std::string &car_name) const
{
    return car_configs.find(car_name) != car_configs.end();
}

std::vector<std::string> CarPhysicsConfig::getAvailableCarTypes() const
{
    std::vector<std::string> types;
    for (const auto &pair : car_configs)
    {
        types.push_back(pair.first);
    }
    return types;
}

void CarPhysicsConfig::loadCarPhysicsFromYAML(CarPhysics &physics, const void *node_ptr)
{
    const YAML::Node &node = *static_cast<const YAML::Node *>(node_ptr);

    // Load turning properties
    if (node["turning"])
    {
        const YAML::Node &turning = node["turning"];
        if (turning["torque"])
            physics.torque = turning["torque"].as<float>();
        if (turning["angular_friction"])
            physics.angular_friction = turning["angular_friction"].as<float>();
        if (turning["angular_damping"])
            physics.angular_damping = turning["angular_damping"].as<float>();
    }

    // Load movement properties
    if (node["movement"])
    {
        const YAML::Node &movement = node["movement"];
        if (movement["max_speed"])
            physics.max_speed = movement["max_speed"].as<float>();
        if (movement["max_acceleration"])
            physics.max_acceleration = movement["max_acceleration"].as<float>();
        if (movement["backward_speed_multiplier"])
            physics.backward_speed_multiplier = movement["backward_speed_multiplier"].as<float>();
        if (movement["speed_controller_gain"])
            physics.speed_controller_gain = movement["speed_controller_gain"].as<float>();
    }

    // Load friction properties
    if (node["friction"])
    {
        const YAML::Node &friction = node["friction"];
        if (friction["max_lateral_impulse"])
            physics.max_lateral_impulse = friction["max_lateral_impulse"].as<float>();
        if (friction["forward_drag_coefficient"])
            physics.forward_drag_coefficient = friction["forward_drag_coefficient"].as<float>();
        if (friction["linear_damping"])
            physics.linear_damping = friction["linear_damping"].as<float>();
    }

    // Load body properties
    if (node["body"])
    {
        const YAML::Node &body = node["body"];
        if (body["density"])
            physics.density = body["density"].as<float>();
        if (body["friction"])
            physics.friction = body["friction"].as<float>();
        if (body["restitution"])
            physics.restitution = body["restitution"].as<float>();
        if (body["width"])
            physics.width = body["width"].as<float>();
        if (body["height"])
            physics.height = body["height"].as<float>();
        if (body["center_offset_y"])
            physics.center_offset_y = body["center_offset_y"].as<float>();
    }
}
