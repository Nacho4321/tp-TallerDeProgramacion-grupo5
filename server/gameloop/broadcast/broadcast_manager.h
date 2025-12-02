#ifndef BROADCAST_MANAGER_H
#define BROADCAST_MANAGER_H

#include <mutex>
#include <unordered_map>
#include <memory>
#include "../../PlayerData.h"
#include "../../../common/queue.h"
#include "../../../common/messages.h"

class BroadcastManager
{
public:
    BroadcastManager(
        std::mutex &players_map_mutex,
        std::unordered_map<int, PlayerData> &players,
        std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> &players_messanger);

    // Send message to all connected players
    void broadcast(ServerMessage &msg);

    // Broadcast GAME_STARTED to all players
    void broadcast_game_started();

    // Broadcast race end times to all players
    void broadcast_race_end_message(uint8_t current_round);

private:
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> &players_messanger;
};

#endif

