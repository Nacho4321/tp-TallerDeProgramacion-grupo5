#include "player_manager.h"
#include "../bridge/bridge_handler.h"
#include "../physics/physics_handler.h"
#include "../../../common/constants.h"
#include <iostream>
#include <algorithm>
#include <chrono>

PlayerManager::PlayerManager(
    std::mutex &players_mutex,
    std::unordered_map<int, PlayerData> &players_ref,
    std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> &messengers,
    std::vector<int> &order,
    WorldManager &world,
    CarPhysicsConfig &physics)
    : players_map_mutex(players_mutex),
      players(players_ref),
      players_messanger(messengers),
      player_order(order),
      world_manager(world),
      physics_config(physics)
{
}

bool PlayerManager::can_add_player(const std::vector<MapLayout::SpawnPointData> &spawn_points) const
{
    if (static_cast<int>(players.size()) >= static_cast<int>(spawn_points.size()))
    {
        std::cout << FULL_LOBBY_MSG << std::endl;
        return false;
    }
    return true;
}

int PlayerManager::add_player_to_order(int player_id)
{
    player_order.push_back(player_id);
    return static_cast<int>(player_order.size()) - 1;
}

void PlayerManager::remove_from_player_order(int client_id)
{
    auto order_it = std::find(player_order.begin(), player_order.end(), client_id);
    if (order_it != player_order.end())
        player_order.erase(order_it);
}

PlayerData PlayerManager::create_default_player_data(int spawn_idx,
                                                     const std::vector<MapLayout::SpawnPointData> &spawn_points)
{
    const MapLayout::SpawnPointData &spawn = spawn_points[spawn_idx];
    std::cout << "[PlayerManager] add_player: assigning spawn point " << spawn_idx
              << " at (" << spawn.x << "," << spawn.y << ")" << std::endl;

    const CarPhysics &car_phys = physics_config.getCarPhysics(GREEN_CAR);

    Position pos = Position{false, spawn.x, spawn.y, not_horizontal, not_vertical, spawn.angle};
    PlayerData player_data;
    player_data.body = world_manager.create_player_body(spawn.x, spawn.y, pos.angle, GREEN_CAR);
    player_data.state = MOVE_UP_RELEASED_STR;
    player_data.car = CarInfo{GREEN_CAR, car_phys.max_speed, car_phys.max_acceleration, car_phys.max_hp, car_phys.collision_damage_multiplier, car_phys.torque};
    player_data.position = pos;
    player_data.next_checkpoint = 0;
    player_data.lap_start_time = std::chrono::steady_clock::now();
    player_data.race_finished = false;
    player_data.god_mode = false;

    return player_data;
}

void PlayerManager::cleanup_player_data(int client_id)
{
    auto it = players.find(client_id);
    if (it == players.end())
        throw std::runtime_error("player not found");

    PlayerData &pd = it->second;
    world_manager.safe_destroy_body(pd.body);

    players_messanger.erase(client_id);
    players.erase(it);
}

void PlayerManager::add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox,
                               const std::vector<MapLayout::SpawnPointData> &spawn_points)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    std::cout << "[PlayerManager] add_player: id=" << id
              << " players.size()=" << players.size()
              << " outbox.valid=" << std::boolalpha << static_cast<bool>(player_outbox)
              << std::endl;

    if (!can_add_player(spawn_points))
        return;

    int spawn_idx = add_player_to_order(id);
    PlayerData player_data = create_default_player_data(spawn_idx, spawn_points);

    players[id] = player_data;
    players_messanger[id] = player_outbox;

    std::cout << "[PlayerManager] add_player: done. players.size()=" << players.size()
              << " messengers.size()=" << players_messanger.size() << std::endl;
}

void PlayerManager::remove_player(int client_id, GameState game_state,
                                  const std::vector<MapLayout::SpawnPointData> &spawn_points)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);

    cleanup_player_data(client_id);
    remove_from_player_order(client_id);

    std::cout << "[PlayerManager] remove_player: client " << client_id << " removed" << std::endl;

    if (game_state == GameState::LOBBY && !player_order.empty())
        reposition_remaining_players(spawn_points);
}

bool PlayerManager::has_player(int client_id) const
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    return players.find(client_id) != players.end();
}

size_t PlayerManager::get_player_count() const
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    return players.size();
}

void PlayerManager::reposition_remaining_players(const std::vector<MapLayout::SpawnPointData> &spawn_points)
{
    std::cout << "[PlayerManager] reordering remaining " << player_order.size() << " players in lobby" << std::endl;

    for (size_t i = 0; i < player_order.size(); ++i)
    {
        int player_id = player_order[i];
        auto player_it = players.find(player_id);
        if (player_it == players.end())
            continue;

        PlayerData &player_data = player_it->second;
        const MapLayout::SpawnPointData &spawn = spawn_points[i];

        std::cout << "[PlayerManager] moving player " << player_id
                  << " to spawn " << i << " at (" << spawn.x << "," << spawn.y << ")" << std::endl;

        world_manager.safe_destroy_body(player_data.body);
        Position new_pos{false, spawn.x, spawn.y, not_horizontal, not_vertical, spawn.angle};
        player_data.body = world_manager.create_player_body(spawn.x, spawn.y, new_pos.angle, player_data.car.car_name);
        player_data.position = new_pos;
    }
}

void PlayerManager::reset_all_players_to_lobby(const std::vector<MapLayout::SpawnPointData> &spawn_points)
{
    for (size_t i = 0; i < player_order.size(); ++i)
    {
        int player_id = player_order[i];
        auto player_it = players.find(player_id);
        if (player_it == players.end())
            continue;

        PlayerData &player_data = player_it->second;
        const MapLayout::SpawnPointData &spawn = spawn_points[i];

        world_manager.safe_destroy_body(player_data.body);
        Position new_pos{false, spawn.x, spawn.y, not_horizontal, not_vertical, spawn.angle};
        player_data.body = world_manager.create_player_body(spawn.x, spawn.y, new_pos.angle, player_data.car.car_name);
        player_data.position = new_pos;

        player_data.next_checkpoint = 0;
        player_data.race_finished = false;
        player_data.is_dead = false;
        player_data.god_mode = false;
        player_data.lap_start_time = std::chrono::steady_clock::now();

        // Reseteo HP
        const CarPhysics &car_physics = physics_config.getCarPhysics(player_data.car.car_name);
        player_data.car.hp = car_physics.max_hp;
    }
}

void PlayerManager::update_body_positions()
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    for (auto &[id, player_data] : players)
    {
        // Si el jugador está muerto, no aplicar fuerzas
        if (player_data.is_dead)
            continue;

        // Si el jugador terminó la carrera, detenerlo completamente
        if (player_data.race_finished)
        {
            if (player_data.body)
            {
                player_data.body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
                player_data.body->SetAngularVelocity(0.0f);
            }
            continue;
        }

        // Si el body está marcado para remover, omitir
        if (player_data.mark_body_for_removal)
            continue;

        if (!player_data.body)
            continue;

        // Aplicar fricción/adhesión primero
        PhysicsHandler::update_friction_for_player(player_data, physics_config);

        // Aplicar fuerza de conducción / torque basado en la entrada
        PhysicsHandler::update_drive_for_player(player_data, physics_config);
    }
}

void PlayerManager::add_player_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast,
                                            int player_id, PlayerData &player_data,
                                            const std::vector<b2Vec2> &checkpoint_centers)
{
    // Si el jugador está muerto y no tiene body, no lo agregamos al broadcast
    if (player_data.is_dead && !player_data.body)
        return;

    b2Body *body = player_data.body;
    if (body)
    {
        b2Vec2 position = body->GetPosition();
        player_data.position.new_X = position.x * SCALE;
        player_data.position.new_Y = position.y * SCALE;
        player_data.position.angle = PhysicsHandler::normalize_angle(body->GetAngle());

        // Actualizar el estado del puente ANTES de copiar la posición
        BridgeHandler::update_bridge_state(player_data);
    }

    PlayerPositionUpdate update;
    update.player_id = player_id;
    update.new_pos = player_data.position;
    update.car_type = player_data.car.car_name;
    update.hp = player_data.car.hp;
    update.collision_flag = player_data.collision_this_frame;
    update.is_stopping = player_data.is_stopping;

    // Enviar niveles de mejora
    update.upgrade_speed = player_data.upgrades.speed;
    update.upgrade_acceleration = player_data.upgrades.acceleration;
    update.upgrade_handling = player_data.upgrades.handling;
    update.upgrade_durability = player_data.upgrades.durability;

    // Solo enviar checkpoints si el jugador no ha terminado la carrera
    if (!player_data.race_finished && !checkpoint_centers.empty())
    {
        int total = static_cast<int>(checkpoint_centers.size());
        for (int k = 0; k < CHECKPOINT_LOOKAHEAD; ++k)
        {
            int idx = player_data.next_checkpoint + k;
            if (idx >= total)
                break;

            b2Vec2 checkpoint_center = checkpoint_centers[idx];
            Position cp_pos{false, checkpoint_center.x * SCALE, checkpoint_center.y * SCALE, not_horizontal, not_vertical, 0.0f};
            update.next_checkpoints.push_back(cp_pos);
        }
    }

    broadcast.push_back(update);
}

void PlayerManager::update_player_positions(std::vector<PlayerPositionUpdate> &broadcast,
                                            const std::vector<b2Vec2> &checkpoint_centers)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);

    for (auto &[id, player_data] : players)
    {
        add_player_to_broadcast(broadcast, id, player_data, checkpoint_centers);
    }
}
