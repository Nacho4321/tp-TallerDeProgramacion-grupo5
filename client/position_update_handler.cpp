#include "position_update_handler.h"
#include <cmath>

PositionUpdateHandler::PositionUpdateHandler()
{
}

CarPosition PositionUpdateHandler::extractCarPosition(const Position& pos) const
{
    double ang = pos.angle;
    return CarPosition{
        pos.new_X,
        pos.new_Y,
        float(-std::sin(ang)),
        float(std::cos(ang)),
        pos.on_bridge
    };
}

PositionUpdateHandler::GameFrame PositionUpdateHandler::processUpdate(
    const ServerMessage& msg,
    size_t idx_main,
    bool player_found,
    int32_t original_player_id,
    const std::unordered_map<std::string, int>& carTypeMap,
    const std::unique_ptr<Car>& mainCar
) const
{
    GameFrame frame;

    frame.mainTypeId = 0;
    frame.mainCarCollisionFlag = false;
    frame.mainCarIsStopping = false;
    frame.mainCarHP = 100.0f;
    frame.upgradeSpeed = 0;
    frame.upgradeAcceleration = 0;
    frame.upgradeHandling = 0;
    frame.upgradeDurability = 0;

    if (player_found)
    {
        const PlayerPositionUpdate &main_pos = msg.positions[idx_main];
        frame.mainCarPosition = extractCarPosition(main_pos.new_pos);

        auto it = carTypeMap.find(main_pos.car_type);
        frame.mainTypeId = (it != carTypeMap.end()) ? it->second : 0;
        frame.next_cps = main_pos.next_checkpoints;
        frame.mainCarCollisionFlag = main_pos.collision_flag;
        frame.mainCarIsStopping = main_pos.is_stopping;
        frame.mainCarHP = main_pos.hp;

        frame.upgradeSpeed = main_pos.upgrade_speed;
        frame.upgradeAcceleration = main_pos.upgrade_acceleration;
        frame.upgradeHandling = main_pos.upgrade_handling;
        frame.upgradeDurability = main_pos.upgrade_durability;
    }
    else if (mainCar)
    {
        frame.mainCarPosition = mainCar->getPosition();
    }

    for (size_t i = 0; i < msg.positions.size(); ++i)
    {
        const PlayerPositionUpdate &pos = msg.positions[i];

        if (pos.player_id == original_player_id)
            continue;

        CarPosition cp = extractCarPosition(pos.new_pos);
        auto other_it = carTypeMap.find(pos.car_type);
        int other_type_id = (other_it != carTypeMap.end()) ? other_it->second : 0;

        frame.otherCars[pos.player_id] = std::make_pair(cp, other_type_id);
        frame.otherCarsCollisionFlags[pos.player_id] = pos.collision_flag;
        frame.otherCarsIsStoppingFlags[pos.player_id] = pos.is_stopping;
    }

    return frame;
}
