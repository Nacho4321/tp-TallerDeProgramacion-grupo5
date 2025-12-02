#include "broadcast_manager.h"
#include "../../../common/constants.h"
#include <iostream>

BroadcastManager::BroadcastManager(
    std::mutex &players_map_mutex,
    std::unordered_map<int, PlayerData> &players,
    std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> &players_messanger)
    : players_map_mutex(players_map_mutex),
      players(players),
      players_messanger(players_messanger)
{
}

void BroadcastManager::broadcast(ServerMessage &msg)
{
    // Snapshot de destinatarios para evitar iterar el mapa mientras puede cambiar
    std::vector<std::pair<int, std::shared_ptr<Queue<ServerMessage>>>> recipients;
    {
        std::lock_guard<std::mutex> lk(players_map_mutex);
        recipients.reserve(players_messanger.size());
        for (auto &entry : players_messanger)
        {
            recipients.emplace_back(entry.first, entry.second);
        }
    }

    std::vector<int> to_remove;
    for (auto &p : recipients)
    {
        int id = p.first;
        auto &queue = p.second;
        if (!queue)
        {
            to_remove.push_back(id);
            continue;
        }
        try
        {
            queue->push(msg);
        }
        catch (const ClosedQueue &)
        {
            // El cliente cerró su outbox: marcar para remover
            to_remove.push_back(id);
        }
    }
    if (!to_remove.empty())
    {
        // Remover jugadores desconectados
        std::lock_guard<std::mutex> lk(players_map_mutex);
        for (int id : to_remove)
        {
            players_messanger.erase(id);
            players.erase(id);
        }
    }
}

void BroadcastManager::broadcast_game_started()
{
    ServerMessage msg;
    msg.opcode = GAME_STARTED;

    for (auto &entry : players_messanger)
    {
        auto &queue = entry.second;
        if (queue)
        {
            try
            {
                queue->push(msg);
                std::cout << "[BroadcastManager] GAME_STARTED sent to player " << entry.first << std::endl;
            }
            catch (const ClosedQueue &)
            {
            }
        }
    }
}

void BroadcastManager::broadcast_race_end_message(uint8_t current_round)
{
    std::cout << "[BroadcastManager] Broadcasting race end message..." << std::endl;
    // Enviar tiempos de la carrera actual a los clientes
    ServerMessage msg;
    msg.opcode = RACE_TIMES;
    {
        std::lock_guard<std::mutex> lk(players_map_mutex);
        uint8_t round_index = current_round;
        for (auto &entry : players)
        {
            int pid = entry.first;
            PlayerData &pd = entry.second;
            uint32_t time_ms = 10u * 60u * 1000u;
            bool dq = pd.disqualified || pd.is_dead;

            int completed_round_idx = pd.rounds_completed - 1;
            if (completed_round_idx >= 0 && completed_round_idx < TOTAL_ROUNDS)
            {
                time_ms = pd.round_times_ms[completed_round_idx];
            }
            msg.race_times.push_back({static_cast<uint32_t>(pid), time_ms, dq, round_index});
        }
    }
    broadcast(msg);
}

