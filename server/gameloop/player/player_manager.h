#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include "../../PlayerData.h"
#include "../../map_layout.h"
#include "../../car_physics_config.h"
#include "../../game_state.h"
#include "../world/world_manager.h"
#include "../../../common/messages.h"
#include "../../../common/queue.h"
#include "../gameloop_constants.h"

class PlayerManager
{
public:
    PlayerManager(
        std::mutex &players_mutex,
        std::unordered_map<int, PlayerData> &players,
        std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> &messengers,
        std::vector<int> &player_order,
        WorldManager &world_manager,
        CarPhysicsConfig &physics_config);

    // Ciclo de vida del player
    void add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox,
                    const std::vector<MapLayout::SpawnPointData> &spawn_points);
    void remove_player(int client_id, GameState game_state,
                       const std::vector<MapLayout::SpawnPointData> &spawn_points);

    // Consultas
    bool has_player(int client_id) const;
    size_t get_player_count() const;
    bool can_add_player(const std::vector<MapLayout::SpawnPointData> &spawn_points) const;

    // Actualizaciones de posición
    void update_body_positions();
    void update_player_positions(std::vector<PlayerPositionUpdate> &broadcast,
                                 const std::vector<b2Vec2> &checkpoint_centers);

    // Reset operations
    void reset_all_players_to_lobby(const std::vector<MapLayout::SpawnPointData> &spawn_points);
    void reposition_remaining_players(const std::vector<MapLayout::SpawnPointData> &spawn_points);

private:
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> &players_messanger;
    std::vector<int> &player_order;
    WorldManager &world_manager;
    CarPhysicsConfig &physics_config;

    // Helpers internos
    int add_player_to_order(int player_id);
    void remove_from_player_order(int client_id);
    PlayerData create_default_player_data(int spawn_idx,
                                          const std::vector<MapLayout::SpawnPointData> &spawn_points);
    void cleanup_player_data(int client_id);
    void add_player_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast,
                                 int player_id, PlayerData &player_data,
                                 const std::vector<b2Vec2> &checkpoint_centers);


};

#endif 
