#ifndef NPC_MANAGER_H
#define NPC_MANAGER_H

#include <vector>
#include <random>
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_math.h>
#include "npc_data.h"
#include "../../map_layout.h"
#include "../../../common/messages.h"

class NPCManager
{
public:
    NPCManager(b2World &world);

    // Inicialización
    void init(const std::vector<MapLayout::ParkedCarData> &parked_data,
              const std::vector<MapLayout::WaypointData> &waypoints);

    // Update del game loop
    void update();

    // Reset para nueva carrera
    void reset_velocities();

    // Para broadcast de posiciones
    void add_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast);

    // Acceso a NPCs (para bridge handler, etc)
    std::vector<NPCData> &get_npcs() { return npcs; }
    const std::vector<NPCData> &get_npcs() const { return npcs; }

private:
    // Constantes
    static constexpr float SCALE = 32.0f;
    static constexpr float NPC_DIRECTION_THRESHOLD = 0.05f;
    static constexpr float NPC_ARRIVAL_THRESHOLD_M = 0.5f;
    static constexpr float MIN_DISTANCE_FROM_PARKED_M = 1.0f;

    b2World &world;
    std::vector<NPCData> npcs;
    std::vector<MapLayout::WaypointData> street_waypoints;

    // Spawn helpers
    void spawn_parked_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data, int &next_negative_id);
    void spawn_moving_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data, int &next_negative_id);
    std::vector<int> get_valid_waypoints_away_from_parked(const std::vector<MapLayout::ParkedCarData> &parked_data);
    int select_closest_waypoint_connection(int start_waypoint_idx);
    float calculate_initial_npc_angle(const b2Vec2 &spawn_pos, const b2Vec2 &target_pos) const;
    NPCData create_moving_npc(int start_idx, int target_idx, float initial_angle, int &next_negative_id);

    // Body creation
    b2Body *create_npc_body(float x_m, float y_m, bool is_static, float angle_rad = 0.0f);

    // Update helpers
    bool should_select_new_waypoint(NPCData &npc, const b2Vec2 &target_pos);
    void select_next_waypoint(NPCData &npc, std::mt19937 &gen);
    void move_npc_towards_target(NPCData &npc, const b2Vec2 &target_pos);

    // Broadcast helper
    void add_npc_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast, NPCData &npc);

    // Utility
    float normalize_angle(double angle) const;
};

#endif 
