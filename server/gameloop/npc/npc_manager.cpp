#include "npc_manager.h"
#include "../../../common/constants.h"
#include "../../npc_config.h"
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_fixture.h>
#include <algorithm>
#include <limits>
#include <iostream>
#include <cmath>

NPCManager::NPCManager(b2World &world)
    : world(world)
{
}

void NPCManager::init(const std::vector<MapLayout::ParkedCarData> &parked_data,
                      const std::vector<MapLayout::WaypointData> &waypoints,
                      const std::vector<MapLayout::SpawnPointData> &spawn_points)
{
    street_waypoints = waypoints;
    player_spawn_points = spawn_points;
    int next_negative_id = -1;
    spawn_parked_npcs(parked_data, next_negative_id);
    spawn_moving_npcs(parked_data, next_negative_id);
}

void NPCManager::update()
{
    if (street_waypoints.empty())
        return;

    std::random_device rd;
    std::mt19937 gen(rd());

    for (auto &npc : npcs)
    {
        b2Body *body = npc.body;
        if (!body || npc.is_parked)
            continue; // NPCs estacionados no se mueven

        // Validar índices
        if (npc.target_waypoint < 0 || npc.target_waypoint >= static_cast<int>(street_waypoints.size()))
            continue;

        b2Vec2 target_pos = street_waypoints[npc.target_waypoint].position;

        // Si llegó al waypoint objetivo, elegir siguiente destino aleatorio
        if (should_select_new_waypoint(npc, target_pos))
        {
            select_next_waypoint(npc, gen);
            target_pos = street_waypoints[npc.target_waypoint].position;
        }

        // Moverse hacia el target
        move_npc_towards_target(npc, target_pos);
    }
}

void NPCManager::reset_velocities()
{
    for (auto &npc : npcs)
    {
        if (!npc.body || npc.is_parked)
            continue;
        npc.body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
    }
}

void NPCManager::add_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast)
{
    for (auto &npc : npcs)
    {
        add_npc_to_broadcast(broadcast, npc);
    }
}

// ============================================================================
// Spawn helpers
// ============================================================================

void NPCManager::spawn_parked_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data, int &next_negative_id)
{
    int parked_count = std::min(static_cast<int>(parked_data.size()), NPCConfig::getInstance().getMaxParked());

    std::random_device rd_parked;
    std::mt19937 gen_parked(rd_parked());

    std::vector<size_t> parked_indices;
    for (size_t i = 0; i < parked_data.size(); ++i)
    {
        parked_indices.push_back(i);
    }
    std::shuffle(parked_indices.begin(), parked_indices.end(), gen_parked);

    for (int i = 0; i < parked_count; ++i)
    {
        const auto &parked = parked_data[parked_indices[i]];
        float angle_rad = parked.horizontal ? b2_pi / 2.0f : 0.0f;
        b2Body *npc_body = create_npc_body(parked.position.x, parked.position.y, true, angle_rad);

        NPCData npc;
        npc.body = npc_body;
        npc.npc_id = next_negative_id--;
        npc.current_waypoint = -1;
        npc.target_waypoint = -1;
        npc.speed_mps = 0.0f;
        npc.is_parked = true;
        npc.is_horizontal = parked.horizontal;
        npc.on_bridge = false;

        npcs.push_back(npc);
    }

    std::cout << "[NPCManager] Spawned " << parked_count << " parked NPCs (out of "
              << parked_data.size() << " available positions)." << std::endl;
}

void NPCManager::spawn_moving_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data, int &next_negative_id)
{
    if (street_waypoints.size() < 2)
    {
        std::cout << "[NPCManager] Not enough waypoints to spawn moving NPCs." << std::endl;
        return;
    }

    std::vector<int> candidate_waypoints = get_valid_waypoints_away_from_parked(parked_data);
    int moving_npcs_count = std::min(NPCConfig::getInstance().getMaxMoving(),
                                     static_cast<int>(candidate_waypoints.size()));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(candidate_waypoints.begin(), candidate_waypoints.end(), gen);

    for (int i = 0; i < moving_npcs_count; ++i)
    {
        int start_wp_idx = candidate_waypoints[i];
        int target_wp_idx = select_closest_waypoint_connection(start_wp_idx);

        float initial_angle = 0.0f;
        if (target_wp_idx != start_wp_idx)
        {
            b2Vec2 spawn_pos = street_waypoints[start_wp_idx].position;
            b2Vec2 target_pos = street_waypoints[target_wp_idx].position;
            initial_angle = calculate_initial_npc_angle(spawn_pos, target_pos);
        }

        NPCData npc = create_moving_npc(start_wp_idx, target_wp_idx, initial_angle, next_negative_id);
        npcs.push_back(npc);
    }

    std::cout << "[NPCManager] Moving NPC spawn candidates: " << candidate_waypoints.size()
              << " chosen: " << moving_npcs_count << std::endl;
    std::cout << "[NPCManager] Spawned " << moving_npcs_count << " moving NPCs." << std::endl;
    std::cout << "[NPCManager] Total NPCs: " << npcs.size() << std::endl;
}

std::vector<int> NPCManager::get_valid_waypoints_away_from_parked(const std::vector<MapLayout::ParkedCarData> &parked_data)
{
    std::vector<int> candidate_waypoints;
    candidate_waypoints.reserve(street_waypoints.size());

    for (int idx = 0; idx < static_cast<int>(street_waypoints.size()); ++idx)
    {
        b2Vec2 wp_pos = street_waypoints[idx].position;
        
        // Verificar distancia a spawn points de jugadores
        if (!should_select_spawn_position(wp_pos))
            continue;

        // Verificar distancia a autos estacionados
        bool too_close_to_parked = false;
        for (const auto &parked_car : parked_data)
        {
            float distance = (wp_pos - parked_car.position).Length();
            if (distance < MIN_DISTANCE_FROM_PARKED_M)
            {
                too_close_to_parked = true;
                break;
            }
        }

        if (!too_close_to_parked)
            candidate_waypoints.push_back(idx);
    }

    if (candidate_waypoints.empty())
    {
        for (int idx = 0; idx < static_cast<int>(street_waypoints.size()); ++idx)
            candidate_waypoints.push_back(idx);
        std::cout << "[NPCManager] Warning: All waypoints filtered out; using full set." << std::endl;
    }

    return candidate_waypoints;
}

bool NPCManager::should_select_spawn_position(const b2Vec2 &waypoint_pos) const
{
    for (const auto &spawn : player_spawn_points)
    {
        // Convertir spawn point de píxeles a metros
        b2Vec2 spawn_pos_m(spawn.x / SCALE, spawn.y / SCALE);
        float distance = (waypoint_pos - spawn_pos_m).Length();
        
        if (distance < MIN_DISTANCE_FROM_SPAWN_M)
            return false;
    }
    return true;
}

int NPCManager::select_closest_waypoint_connection(int start_waypoint_idx)
{
    const MapLayout::WaypointData &start_wp = street_waypoints[start_waypoint_idx];
    b2Vec2 spawn_pos = start_wp.position;

    if (start_wp.connections.empty())
        return start_waypoint_idx;

    float best_dist = std::numeric_limits<float>::max();
    int closest_idx = start_waypoint_idx;

    for (int candidate_idx : start_wp.connections)
    {
        if (candidate_idx < 0 || candidate_idx >= static_cast<int>(street_waypoints.size()))
            continue;

        b2Vec2 candidate_pos = street_waypoints[candidate_idx].position;
        float distance = (candidate_pos - spawn_pos).Length();

        if (distance < best_dist)
        {
            best_dist = distance;
            closest_idx = candidate_idx;
        }
    }

    return closest_idx;
}

float NPCManager::calculate_initial_npc_angle(const b2Vec2 &spawn_pos, const b2Vec2 &target_pos) const
{
    b2Vec2 direction = target_pos - spawn_pos;
    float length = direction.Length();

    if (length <= 0.0001f)
        return 0.0f;

    float movement_angle = std::atan2(direction.y, direction.x);
    return movement_angle - b2_pi / 2.0f; // sprite orientado hacia arriba
}

NPCData NPCManager::create_moving_npc(int start_idx, int target_idx, float initial_angle, int &next_negative_id)
{
    b2Vec2 spawn_pos = street_waypoints[start_idx].position;
    float speed_mps = NPCConfig::getInstance().getSpeedPxS() / SCALE;

    b2Body *npc_body = create_npc_body(spawn_pos.x, spawn_pos.y, false, initial_angle);

    NPCData npc;
    npc.body = npc_body;
    npc.npc_id = next_negative_id--;
    npc.current_waypoint = start_idx;
    npc.target_waypoint = target_idx;
    npc.speed_mps = speed_mps;
    npc.is_parked = false;
    npc.is_horizontal = false;

    return npc;
}

// Body creation

b2Body *NPCManager::create_npc_body(float x_m, float y_m, bool is_static, float angle_rad)
{
    b2BodyDef bd;
    bd.type = is_static ? b2_staticBody : b2_dynamicBody;
    bd.position.Set(x_m, y_m);
    bd.angle = angle_rad;
    b2Body *npc_body = world.CreateBody(&bd);

    float halfWidth = 22.0f / (2.0f * SCALE);
    float halfHeight = 28.0f / (2.0f * SCALE);
    b2PolygonShape shape;
    shape.SetAsBox(halfWidth, halfHeight);
    b2FixtureDef fd;
    fd.shape = &shape;
    fd.isSensor = false;
    fd.density = 1.0f;
    fd.friction = 0.3f;
    fd.restitution = 0.0f;

    fd.filter.categoryBits = CAR_GROUND;

    fd.filter.maskBits =
        COLLISION_FLOOR |     // Colisiones del suelo
        CAR_GROUND |          // Otros jugadores en el suelo
        SENSOR_START_BRIDGE | // Sensores de entrada
        SENSOR_END_BRIDGE;    // Sensores de salida

    npc_body->CreateFixture(&fd);
    return npc_body;
}

// Update helpers

bool NPCManager::should_select_new_waypoint(NPCData &npc, const b2Vec2 &target_pos)
{
    b2Vec2 pos = npc.body->GetPosition();
    float dist = (target_pos - pos).Length();
    return dist < NPC_ARRIVAL_THRESHOLD_M;
}

void NPCManager::select_next_waypoint(NPCData &npc, std::mt19937 &gen)
{
    npc.current_waypoint = npc.target_waypoint;
    const MapLayout::WaypointData &current_wp = street_waypoints[npc.current_waypoint];

    // Elegir aleatoriamente uno de los waypoints conectados
    if (!current_wp.connections.empty())
    {
        std::uniform_int_distribution<size_t> conn_dist(0, current_wp.connections.size() - 1);
        npc.target_waypoint = current_wp.connections[conn_dist(gen)];
    }
}

void NPCManager::move_npc_towards_target(NPCData &npc, const b2Vec2 &target_pos)
{
    b2Body *body = npc.body;
    b2Vec2 pos = body->GetPosition();
    b2Vec2 to_target = target_pos - pos;
    float dist = to_target.Length();

    if (dist > 0.0001f)
    {
        b2Vec2 dir = (1.0f / dist) * to_target; // normalizar
        b2Vec2 vel = npc.speed_mps * dir;
        body->SetLinearVelocity(vel);

        // Orientar el auto en la dirección del movimiento
        float movement_angle = std::atan2(vel.y, vel.x);
        float body_angle = movement_angle - b2_pi / 2.0f; // ajustar por sprite orientado hacia arriba
        body->SetTransform(pos, body_angle);
    }
    else
    {
        body->SetLinearVelocity(b2Vec2(0, 0));
    }
}

// Broadcast helper

void NPCManager::add_npc_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast, NPCData &npc)
{
    if (!npc.body)
        return;

    b2Vec2 position = npc.body->GetPosition();
    Position pos{};
    pos.new_X = position.x * SCALE;
    pos.new_Y = position.y * SCALE;

    b2Vec2 velocity = npc.body->GetLinearVelocity();
    MovementDirectionX dx = not_horizontal;
    MovementDirectionY dy = not_vertical;
    if (velocity.x > NPC_DIRECTION_THRESHOLD)
        dx = right;
    else if (velocity.x < -NPC_DIRECTION_THRESHOLD)
        dx = left;
    if (velocity.y > NPC_DIRECTION_THRESHOLD)
        dy = down;
    else if (velocity.y < -NPC_DIRECTION_THRESHOLD)
        dy = up;
    pos.direction_x = dx;
    pos.direction_y = dy;
    pos.angle = normalize_angle(npc.body->GetAngle());
    pos.on_bridge = npc.on_bridge;

    PlayerPositionUpdate update;
    update.player_id = npc.npc_id; // id negativo para NPC
    update.new_pos = pos;
    update.car_type = "npc";
    update.hp = 100.0f;
    update.collision_flag = false;
    broadcast.push_back(update);
}

// Utility

float NPCManager::normalize_angle(double angle) const
{
    while (angle < 0.0)
        angle += 2.0 * M_PI;
    while (angle >= 2.0 * M_PI)
        angle -= 2.0 * M_PI;
    return static_cast<float>(angle);
}
