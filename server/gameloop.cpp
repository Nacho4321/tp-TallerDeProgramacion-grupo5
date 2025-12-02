#include "gameloop.h"
#include "../common/constants.h"
#include "npc_config.h"
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#include <iomanip>
#define INITIAL_X_POS 960
#define INITIAL_Y_POS 540
#define FULL_LOBBY_MSG "can't join lobby, maximum players reached"
#define SCALE 32.0f
#define FPS (1.0f / 60.0f)
#define VELOCITY_ITERS 8
#define COLLISION_ITERS 3
// JSON loader
#include <nlohmann/json.hpp>
#include <fstream>
#include <limits>

//
//
// Refactor: Mapas y checkpoints
//
//

void GameLoop::setup_npc_config()
{
    auto &npc_cfg = NPCConfig::getInstance();
    npc_cfg.loadFromFile("config/npc.yaml");
}

void GameLoop::setup_world()
{
    std::vector<MapLayout::ParkedCarData> parked_data;
    std::vector<MapLayout::WaypointData> street_waypoints;
    setup_map_layout();
    load_current_round_checkpoints();
    setup_npc_config();

    uint8_t safe_map_id = (map_id < MAP_COUNT) ? map_id : 0;
    map_layout.extract_map_npc_data(MAP_JSON_PATHS[safe_map_id], street_waypoints, parked_data);
    if (!parked_data.empty() || !street_waypoints.empty())
    {
        npc_manager.init(parked_data, street_waypoints, spawn_points);
    }
}

void GameLoop::setup_map_layout()
{
    uint8_t safe_map_id = (map_id < MAP_COUNT) ? map_id : 0;
    map_layout.create_map_layout(MAP_JSON_PATHS[safe_map_id]);
}

void GameLoop::load_current_round_checkpoints()
{
    CheckpointHandler::load_round_checkpoints(
        current_round,
        checkpoint_sets,
        world_manager.get_world(),
        map_layout,
        checkpoint_centers,
        checkpoint_fixtures);
}

//
//
// Refactor: Logica del game loop
//
//

void GameLoop::run()
{
    auto last_tick = std::chrono::steady_clock::now();
    float acum = 0.0f;

    setup_world();

    while (should_keep_running())
    {
        try
        {
            // Falla segura: antes de procesar eventos, validar si el countdown terminó
            // para evitar quedar bloqueados por procesamiento de eventos.
            maybe_finish_starting_and_play();

            event_loop.process_available_events(game_state);

            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - last_tick).count();
            last_tick = now;
            acum += dt;

            if (game_state == GameState::PLAYING)
            {
                process_playing_state(acum);
            }
            else if (game_state == GameState::LOBBY)
            {
                process_lobby_state();
            }
            else if (game_state == GameState::STARTING)
            {
                process_starting_state();
                // Falla segura: aunque por algún motivo no se ejecute el process,
                // chequeamos igualmente la finalización del countdown en el loop.
                maybe_finish_starting_and_play();
            }
        }
        catch (const ClosedQueue &)
        {
            // Ignorar: alguna cola de cliente cerrada; ya se limpia en broadcast_positions
        }
        catch (const std::exception &e)
        {
            std::cerr << "[GameLoop] Unexpected exception: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void GameLoop::process_playing_state(float &acum)
{

    if (players.empty())
        return;

    if (reset_accumulator.load())
    {
        acum = 0.0f;
        reset_accumulator.store(false);
        std::cout << "[GameLoop] Physics accumulator reset on start." << std::endl;
    }
    RaceManager::check_round_timeout(players, game_state, round_timeout_checked, round_start_time, pending_race_reset);

    // Reset collision flags at the start of each frame
    for (auto &entry : players)
    {
        entry.second.collision_this_frame = false;
    }

    npc_manager.update();
    update_body_positions();

    while (acum >= FPS)
    {
        world_manager.step(FPS, VELOCITY_ITERS, COLLISION_ITERS);
        acum -= FPS;
    }

    // Flush de destrucciones diferidas (un solo lugar). El mundo ya no está locked.
    {
        std::lock_guard<std::mutex> lk(players_map_mutex);
        for (auto &[id, player_data] : players)
        {
            // Procesar cheat de completar ronda pendiente
            if (player_data.pending_race_complete && !player_data.race_finished)
            {
                RaceManager::complete_player_race(player_data, pending_race_reset, players);
                player_data.pending_race_complete = false;
            }

            // Procesar cheat de descalificación pendiente
            if (player_data.pending_disqualification && !player_data.is_dead)
            {
                CollisionHandler::disqualify_player(player_data);
                RaceManager::check_race_completion(players, pending_race_reset);
                player_data.pending_disqualification = false;
            }

            if (player_data.mark_body_for_removal && player_data.body)
            {
                if (world_manager.is_locked())
                {
                    // Esto no debería pasar acá, pero dejo el log por si aparece alguna condición rara.
                    std::cout << "[GameLoop] World locked during flush, postergando destroy para player " << id << std::endl;
                    continue;
                }
                world_manager.safe_destroy_body(player_data.body);
                player_data.mark_body_for_removal = false;
                std::cout << "[GameLoop] Destroyed body for player " << id << " (flush)." << std::endl;
            }
        }
    }

    perform_race_reset();

    std::vector<PlayerPositionUpdate> broadcast;
    update_player_positions(broadcast);

    ServerMessage msg;
    msg.opcode = UPDATE_POSITIONS;
    msg.positions = broadcast;
    broadcast_positions(msg);
}

void GameLoop::process_lobby_state()
{
    if (players.empty())
        return;

    // Reset collision flags at the start of each frame
    for (auto &entry : players)
    {
        entry.second.collision_this_frame = false;
    }
}

void GameLoop::process_starting_state()
{
    // Durante STARTING igual avanzamos la cuenta regresiva aunque no haya jugadores,
    // para evitar quedar trabados en este estado.

    // Reset collision flags at the start of each frame
    for (auto &entry : players)
    {
        entry.second.collision_this_frame = false;
    }

    std::vector<PlayerPositionUpdate> broadcast;
    update_player_positions(broadcast);

    ServerMessage msg;
    msg.opcode = UPDATE_POSITIONS;
    msg.positions = broadcast;
    broadcast_positions(msg);

    // Checkeamos si la cuenta regresiva terminó
    maybe_finish_starting_and_play();
}

void GameLoop::perform_race_reset()
{
    // Evitar realizar operaciones pesadas (destroy/recreate bodies, reload checkpoints)
    // bajo el lock del mapa de jugadores. Primero chequeamos el flag y salimos del lock.
    bool do_reset = false;
    {
        std::lock_guard<std::mutex> lk(players_map_mutex);
        if (RaceManager::should_reset_race(pending_race_reset))
        {
            // Limpiamos el flag acá para que no se vuelva a ejecutar en el mismo frame.
            pending_race_reset.store(false);
            do_reset = true;
        }
    }

    if (!do_reset)
        return;

    broadcast_race_end_message();
    advance_round_or_reset_to_lobby();
}

void GameLoop::advance_round_or_reset_to_lobby()
{
    // Nueva política: NO volver a LOBBY entre rondas.
    // Avanzar de ronda (o reiniciar campeonato) y entrar directamente en STARTING (cuenta regresiva) automáticamente.
    if (current_round < 2)
    {
        current_round++;
        std::cout << "[GameLoop] Advancing to round " << (current_round + 1) << " of 3 (auto STARTING)" << std::endl;

        // Preparar siguiente ronda: recargar checkpoints
        load_current_round_checkpoints();

        // Reposicionar jugadores a los spawns y limpiar estado de carrera
        reset_all_players_to_lobby();

        // Limpiar flags y pasar a STARTING
        pending_race_reset.store(false);
        std::cout << "[GameLoop] About to call transition_to_starting_state for round " << (current_round + 1) << std::endl;
        transition_to_starting_state(10);
    }
    else
    {
        // Terminar campeonato: antes de reiniciar, enviar TOTAL_TIMES
        ServerMessage totals;
        totals.opcode = TOTAL_TIMES;
        {
            std::lock_guard<std::mutex> lk(players_map_mutex);
            for (auto &entry : players)
            {
                totals.total_times.push_back({static_cast<uint32_t>(entry.first), entry.second.total_time_ms});
            }
        }
        broadcast_positions(totals);

        // Reiniciar al round 1 y entrar a STARTING directamente
        std::cout << "[GameLoop] Championship finished. Restarting at round 1 (auto STARTING)." << std::endl;
        current_round = 0;
        load_current_round_checkpoints();
        reset_all_players_to_lobby();
        pending_race_reset.store(false);
        std::cout << "[GameLoop] About to call transition_to_starting_state for championship restart" << std::endl;
        transition_to_starting_state(10);
    }
}

// Constructor para poder setear el contact listener del world
GameLoop::GameLoop(std::shared_ptr<Queue<Event>> events, uint8_t map_id_param)
    : world_manager(CarPhysicsConfig::getInstance()), players_map_mutex(), players(), players_messanger(), event_queue(events), event_loop(players_map_mutex, players, event_queue), started(false), game_state(GameState::LOBBY), next_id(INITIAL_ID), map_id(map_id_param), map_layout(world_manager.get_world()), npc_manager(world_manager.get_world()), physics_config(CarPhysicsConfig::getInstance())
{
    if (!physics_config.loadFromFile("config/car_physics.yaml"))
    {
        std::cerr << "[GameLoop] WARNING: Failed to load car physics config, using defaults" << std::endl;
    }

    // Inicializar rutas según el mapa seleccionado
    uint8_t safe_map_id = (map_id < MAP_COUNT) ? map_id : 0;
    for (int i = 0; i < 3; ++i)
    {
        checkpoint_sets[i] = MAP_CHECKPOINT_PATHS[safe_map_id][i];
    }

    // Cargar spawn points del mapa correspondiente
    map_layout.extract_spawn_points(MAP_SPAWN_POINTS_PATHS[safe_map_id], spawn_points);

    std::cout << "[GameLoop] Initialized with map_id=" << int(map_id)
              << " (" << MAP_NAMES[safe_map_id] << ")" << std::endl;

    // Configurar el callback de contacto
    world_manager.set_contact_callback([this](b2Fixture *a, b2Fixture *b) {
        this->handle_begin_contact(a, b);
    });
}

void GameLoop::transition_to_lobby_state()
{
    game_state = GameState::LOBBY;
    pending_race_reset.store(false);
    std::cout << "[GameLoop] Race reset complete. Returning to LOBBY." << std::endl;
}

void GameLoop::transition_to_starting_state(int countdown_seconds)
{
    starting_active = true;
    game_state = GameState::STARTING;
    starting_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(countdown_seconds);
    std::cout << "[GameLoop] STARTING: countdown " << countdown_seconds << "s before PLAYING" << std::endl;

    // Notificar a los clientes el inicio de la cuenta regresiva
    ServerMessage msg;
    msg.opcode = STARTING_COUNTDOWN;
    broadcast_positions(msg);
}

void GameLoop::maybe_finish_starting_and_play()
{
    if (!starting_active)
    {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if (now >= starting_deadline)
    {
        starting_active = false;
        std::cout << "[GameLoop] STARTING finished. Transitioning to PLAYING" << std::endl;
        transition_to_playing_state();
    }
}

void GameLoop::transition_to_playing_state()
{
    game_state = GameState::PLAYING;
    std::cout << "[GameLoop] Game started! Transitioning from LOBBY to PLAYING" << std::endl;
    reset_accumulator.store(true);

    auto race_start_time = std::chrono::steady_clock::now();
    for (auto &[id, player_data] : players)
    {
        player_data.lap_start_time = race_start_time;
    }

    // Iniciar contador de 10 minutos para la ronda
    round_start_time = race_start_time;
    round_timeout_checked = false;
    std::cout << "[GameLoop] Race timer started for all players" << std::endl;

    broadcast_game_started();
}

void GameLoop::start_game()
{
    bool can_start = false;

    // Scope limitado para el lock: solo para leer estado y modificar datos de jugadores
    {
        std::lock_guard<std::mutex> lk(players_map_mutex);

        // Checkeo si se puede iniciar la carrera
        if (game_state == GameState::LOBBY)
        {
            can_start = true;
        }
        else if (game_state == GameState::PLAYING)
        {
            // Durante PLAYING, solo permitir reiniciar si todos los jugadores están muertos
            int total_players = static_cast<int>(players.size());
            int dead_count = 0;

            for (const auto &[id, player_data] : players)
            {
                if (player_data.is_dead)
                {
                    dead_count++;
                }
            }

            if (dead_count == total_players && total_players > 0)
            {
                can_start = true;
                std::cout << "[GameLoop] All players dead, restarting race..." << std::endl;
            }
        }

        if (!can_start)
        {
            std::cout << "[GameLoop] Cannot start game in current state" << std::endl;
            return;
        }

        // Al iniciar una carrera explícitamente, limpiar cualquier reset pendiente
        // para evitar que perform_race_reset() dispare inmediatamente.
        pending_race_reset.store(false);
        // Asegurar que los checkpoints de la ronda actual estén cargados al iniciar
        load_current_round_checkpoints();
        RaceManager::reset_players_for_race_start(players, physics_config);
        npc_manager.reset_velocities();
    } // <-- Lock se libera aquí

    // Cambiar el estado FUERA del lock para evitar deadlock con el game loop
    transition_to_starting_state(10);
    started = true;
}

void GameLoop::reset_all_players_to_lobby()
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
        player_data.god_mode = false; // Reset god mode para nueva ronda
        player_data.lap_start_time = std::chrono::steady_clock::now();

        // Reseteo HP
        const CarPhysics &car_physics = physics_config.getCarPhysics(player_data.car.car_name);
        player_data.car.hp = car_physics.max_hp;
    }
}

//
//
// Refactor: Broadcasts y updates
//
//

void GameLoop::broadcast_game_started()
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
                std::cout << "[GameLoop] GAME_STARTED sent to player " << entry.first << std::endl;
            }
            catch (const ClosedQueue &)
            {
            }
        }
    }
}

void GameLoop::broadcast_positions(ServerMessage &msg)
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

void GameLoop::broadcast_race_end_message()
{
    std::cout << "[GameLoop] Executing race reset..." << std::endl;
    // Enviar tiempos de la carrera actual a los clientes
    ServerMessage msg;
    msg.opcode = RACE_TIMES;
    {
        std::lock_guard<std::mutex> lk(players_map_mutex);
        uint8_t round_index = static_cast<uint8_t>(current_round);
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
    broadcast_positions(msg);
}

void GameLoop::add_player_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast, int player_id, PlayerData &player_data)
{
    // Si el jugador está muerto y no tiene body, no lo agregamos al broadcast
    // (el auto desaparece visualmente)
    if (player_data.is_dead && !player_data.body)
    {
        return;
    }

    b2Body *body = player_data.body;
    if (body)
    {
        b2Vec2 position = body->GetPosition();
        // Actualizar posición básica
        player_data.position.new_X = position.x * SCALE;
        player_data.position.new_Y = position.y * SCALE;
        player_data.position.angle = PhysicsHandler::normalize_angle(body->GetAngle());

        // IMPORTANTE: Actualizar el estado del puente ANTES de copiar la posición
        BridgeHandler::update_bridge_state(player_data);
    }

    // Ahora crear el update con los datos correctos
    PlayerPositionUpdate update;
    update.player_id = player_id;
    update.new_pos = player_data.position; // Ahora incluye on_bridge actualizado
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

            // Stop at the last checkpoint, don't wrap around
            if (idx >= total)
                break;

            b2Vec2 checkpoint_center = checkpoint_centers[idx];
            Position cp_pos{false, checkpoint_center.x * SCALE, checkpoint_center.y * SCALE, not_horizontal, not_vertical, 0.0f};
            update.next_checkpoints.push_back(cp_pos);
        }
    }

    broadcast.push_back(update);
}

void GameLoop::update_body_positions()
{
    // Velocidad del body (en metros/seg)
    std::lock_guard<std::mutex> lk(players_map_mutex);
    for (auto &[id, player_data] : players)
    {
        // Si el jugador está muerto, no aplicar fuerzas
        if (player_data.is_dead)
        {
            continue;
        }

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

        // Si el body está marcado para remover, NO lo destruyo acá (se hace en flush post-Step)
        // Simplemente omito actualizar fuerzas/física para este jugador.
        if (player_data.mark_body_for_removal)
        {
            continue; // se destruye en el flush dentro de process_playing_state
        }
        if (!player_data.body)
            continue;
        // Aplicar fricción/adhesión primero
        PhysicsHandler::update_friction_for_player(player_data, physics_config);

        // Aplicar fuerza de conducción / torque basado en la entrada
        PhysicsHandler::update_drive_for_player(player_data, physics_config);
    }
}

void GameLoop::update_player_positions(std::vector<PlayerPositionUpdate> &broadcast)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);

    for (auto &[id, player_data] : players)
    {
        add_player_to_broadcast(broadcast, id, player_data);
    }

    // Actualizar estado del bridge para NPCs antes de agregar al broadcast
    for (auto &npc : npc_manager.get_npcs())
    {
        BridgeHandler::update_bridge_state(npc);
    }

    npc_manager.add_to_broadcast(broadcast);
}

//
//
// Refactor: Logica de colisiones y fisicas
//
//

void GameLoop::process_pair(b2Fixture *maybePlayerFix, b2Fixture *maybeCheckpointFix)
{
    int player_id, checkpoint_index;
    if (!CheckpointHandler::is_valid_checkpoint_collision(
            maybePlayerFix, maybeCheckpointFix, players, checkpoint_fixtures,
            player_id, checkpoint_index))
        return;

    PlayerData &player_data = players[player_id];
    int total = static_cast<int>(checkpoint_centers.size());
    bool completed_lap = CheckpointHandler::handle_checkpoint_reached(
        player_data, player_id, checkpoint_index, total);
    
    if (completed_lap)
    {
        RaceManager::complete_player_race(player_data, pending_race_reset, players);
    }
}

void GameLoop::handle_begin_contact(b2Fixture *fixture_a, b2Fixture *fixture_b)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);

    // Checkeo checkpoint
    process_pair(fixture_a, fixture_b);
    process_pair(fixture_b, fixture_a);

    // Checkeo colisiones entre autos
    bool any_death = CollisionHandler::handle_car_collision(fixture_a, fixture_b, players, game_state);
    if (any_death)
    {
        RaceManager::check_race_completion(players, pending_race_reset);
    }
}

PlayerData GameLoop::create_default_player_data(int spawn_idx)
{
    const MapLayout::SpawnPointData &spawn = spawn_points[spawn_idx];
    std::cout << "[GameLoop] add_player: assigning spawn point " << spawn_idx
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

void GameLoop::add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    std::cout << "[GameLoop] add_player: id=" << id
              << " players.size()=" << players.size()
              << " outbox.valid=" << std::boolalpha << static_cast<bool>(player_outbox)
              << std::endl;

    if (!can_add_player())
        return;

    int spawn_idx = add_player_to_order(id);
    PlayerData player_data = create_default_player_data(spawn_idx);

    players[id] = player_data;
    players_messanger[id] = player_outbox;

    std::cout << "[GameLoop] add_player: done. players.size()=" << players.size()
              << " messengers.size()=" << players_messanger.size() << std::endl;
}

void GameLoop::cleanup_player_data(int client_id)
{
    auto it = players.find(client_id);
    if (it == players.end())
        throw std::runtime_error("player not found");

    PlayerData &pd = it->second;
    world_manager.safe_destroy_body(pd.body);

    players_messanger.erase(client_id);
    players.erase(it);
}

void GameLoop::reposition_remaining_players()
{
    std::cout << "[GameLoop] remove_player: reordering remaining " << player_order.size() << " players in lobby" << std::endl;

    for (size_t i = 0; i < player_order.size(); ++i)
    {
        int player_id = player_order[i];
        auto player_it = players.find(player_id);
        if (player_it == players.end())
            continue;

        PlayerData &player_data = player_it->second;
        const MapLayout::SpawnPointData &spawn = spawn_points[i];

        std::cout << "[GameLoop] remove_player: moving player " << player_id
                  << " to spawn " << i << " at (" << spawn.x << "," << spawn.y << ")" << std::endl;

        world_manager.safe_destroy_body(player_data.body);
        Position new_pos{false, spawn.x, spawn.y, not_horizontal, not_vertical, spawn.angle};
        player_data.body = world_manager.create_player_body(spawn.x, spawn.y, new_pos.angle, player_data.car.car_name);
        player_data.position = new_pos;
    }
}

//
//
// Refactor: multipartidas
//
//

size_t GameLoop::get_player_count() const
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    return players.size();
}

bool GameLoop::has_player(int client_id) const
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    return players.find(client_id) != players.end();
}

bool GameLoop::is_joinable() const
{
    return game_state == GameState::LOBBY;
}

bool GameLoop::can_add_player() const
{
    if (static_cast<int>(players.size()) >= static_cast<int>(spawn_points.size()))
    {
        std::cout << FULL_LOBBY_MSG << std::endl;
        return false;
    }
    return true;
}

int GameLoop::add_player_to_order(int player_id)
{
    player_order.push_back(player_id);
    return static_cast<int>(player_order.size()) - 1;
}

void GameLoop::remove_from_player_order(int client_id)
{
    auto order_it = std::find(player_order.begin(), player_order.end(), client_id);
    if (order_it != player_order.end())
        player_order.erase(order_it);
}

void GameLoop::remove_player(int client_id)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);

    cleanup_player_data(client_id);
    remove_from_player_order(client_id);

    std::cout << "[GameLoop] remove_player: client " << client_id << " removed" << std::endl;

    if (game_state == GameState::LOBBY && !player_order.empty())
        reposition_remaining_players();
}
