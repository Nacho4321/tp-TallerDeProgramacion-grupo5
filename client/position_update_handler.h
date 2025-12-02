#ifndef POSITION_UPDATE_HANDLER_H
#define POSITION_UPDATE_HANDLER_H

#include "../common/messages.h"
#include "car.h"
#include <map>
#include <unordered_map>
#include <string>
#include <memory>


class PositionUpdateHandler {
public:
    struct GameFrame {
        CarPosition mainCarPosition;
        int mainTypeId;
        std::vector<Position> next_cps;
        bool mainCarCollisionFlag;
        bool mainCarIsStopping;
        float mainCarHP;
        std::map<int, std::pair<CarPosition, int>> otherCars;
        std::map<int, bool> otherCarsCollisionFlags;
        std::map<int, bool> otherCarsIsStoppingFlags;

        uint8_t upgradeSpeed;
        uint8_t upgradeAcceleration;
        uint8_t upgradeHandling;
        uint8_t upgradeDurability;
    };

    PositionUpdateHandler();

    GameFrame processUpdate(
        const ServerMessage& msg,
        size_t idx_main,
        bool player_found,
        int32_t original_player_id,
        const std::unordered_map<std::string, int>& carTypeMap,
        const std::unique_ptr<Car>& mainCar 
    ) const;

private:
    CarPosition extractCarPosition(const Position& pos) const;
};

#endif // POSITION_UPDATE_HANDLER_H
