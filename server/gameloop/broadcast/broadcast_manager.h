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

    // Envio mensaje a todos los jugadores conectados
    void broadcast(ServerMessage &msg);

    // Envio mensaje GAME_STARTED a todos los jugadores
    void broadcast_game_started();

    // Envio mensaje de tiempos de carrera a todos los jugadores
    void broadcast_race_end_message(uint8_t current_round);

private:
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> &players_messanger;
};

#endif

