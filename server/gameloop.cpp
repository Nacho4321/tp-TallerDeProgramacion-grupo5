#include "gameloop.h"
#include "../common/constants.h"
#include <thread>
#include <chrono>
#include <iostream>

void GameLoop::run()
{
    auto last_tick = std::chrono::steady_clock::now();
    float acum = 0.0f;

    state_manager.set_on_starting_callback([this]() {
        ServerMessage msg;
        msg.opcode = STARTING_COUNTDOWN;
        broadcast_manager.broadcast(msg);
    });
    state_manager.set_on_playing_callback([this]() {
        on_playing_started();
    });

    setup_manager.setup_world(current_round);

    while (should_keep_running())
    {
        try
        {
            state_manager.check_and_finish_starting();
            event_loop.process_available_events(state_manager.get_state());

            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - last_tick).count();
            last_tick = now;
            acum += dt;

            tick_processor.process(state_manager.get_state(), acum);
            perform_race_reset();
        }
        catch (const ClosedQueue &)
        {
        }
        catch (const std::exception &e)
        {
            std::cerr << "[GameLoop] Unexpected exception: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void GameLoop::perform_race_reset()
{
    bool do_reset = false;
    {
        std::lock_guard<std::mutex> lk(players_map_mutex);
        if (RaceManager::should_reset_race(state_manager.get_pending_race_reset()))
        {
            // Limpiamos el flag acá para que no se vuelva a ejecutar en el mismo frame.
            state_manager.get_pending_race_reset().store(false);
            do_reset = true;
        }
    }

    if (!do_reset)
        return;

    broadcast_manager.broadcast_race_end_message(static_cast<uint8_t>(current_round));
    advance_round_or_reset_to_lobby();
}

void GameLoop::advance_round_or_reset_to_lobby()
{
    // Avanzar de ronda (o reiniciar campeonato) y entrar directamente en STARTING (cuenta regresiva) automáticamente.
    if (current_round < 2)
    {
        current_round++;

        // Preparar siguiente ronda: recargar checkpoints
        setup_manager.load_checkpoints(current_round);

        // Reposicionar jugadores a los spawns y limpiar estado de carrera
        player_manager.reset_all_players_to_lobby(spawn_points);

        // Limpiar flags y pasar a STARTING
        state_manager.get_pending_race_reset().store(false);
        state_manager.transition_to_starting(10);
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
        broadcast_manager.broadcast(totals);

        // Reiniciar al round 1 y entrar a STARTING directamente
        current_round = 0;
        setup_manager.load_checkpoints(current_round);
        player_manager.reset_all_players_to_lobby(spawn_points);
        state_manager.get_pending_race_reset().store(false);
        state_manager.transition_to_starting(10);
    }
}

// Constructor para poder setear el contact listener del world
GameLoop::GameLoop(std::shared_ptr<Queue<Event>> events, uint8_t map_id_param)
    : world_manager(CarPhysicsConfig::getInstance()), players_map_mutex(), players(), players_messanger(), event_queue(events), event_loop(players_map_mutex, players, event_queue), started(false), state_manager(), next_id(INITIAL_ID), map_id(map_id_param), map_layout(world_manager.get_world()), npc_manager(world_manager.get_world()), physics_config(CarPhysicsConfig::getInstance()), player_manager(players_map_mutex, players, players_messanger, player_order, world_manager, physics_config), broadcast_manager(players_map_mutex, players, players_messanger), tick_processor(players_map_mutex, players, state_manager, player_manager, npc_manager, world_manager, broadcast_manager, checkpoint_centers), contact_handler(players_map_mutex, players, checkpoint_fixtures, checkpoint_centers, state_manager.get_pending_race_reset(), [this]() { return state_manager.get_state(); }), setup_manager(map_id, map_layout, world_manager, npc_manager, checkpoint_sets, spawn_points, checkpoint_fixtures, checkpoint_centers)
{
    if (!physics_config.loadFromFile(std::string(CONFIG_DIR) + "/car_physics.yaml"))
    {
        std::cerr << "[GameLoop] WARNING: Failed to load car physics config, using defaults" << std::endl;
    }

    // Inicializar rutas según el mapa seleccionado
    uint8_t safe_map_id = (map_id < MAP_COUNT) ? map_id : 0;
    for (int i = 0; i < 3; ++i)
    {
        checkpoint_sets[i] = getMapCheckpointPath(safe_map_id, i);
    }

    // Cargar spawn points del mapa correspondiente
    map_layout.extract_spawn_points(getMapSpawnPointsPath(safe_map_id), spawn_points);

    // Configurar el callback de contacto
    world_manager.set_contact_callback([this](b2Fixture *a, b2Fixture *b) {
        this->contact_handler.handle_begin_contact(a, b);
    });
}

void GameLoop::on_playing_started()
{
    auto race_start_time = std::chrono::steady_clock::now();
    for (auto &[id, player_data] : players)
    {
        player_data.lap_start_time = race_start_time;
    }
    broadcast_manager.broadcast_game_started();
}

void GameLoop::start_game()
{
    bool can_start = false;

    {
        std::lock_guard<std::mutex> lk(players_map_mutex);

        // Checkeo si se puede iniciar la carrera
        if (state_manager.get_state() == GameState::LOBBY)
        {
            can_start = true;
        }
        else if (state_manager.is_playing())
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
            }
        }

        if (!can_start)
        {
            return;
        }

        // Al iniciar una carrera explícitamente, limpiar cualquier reset pendiente
        // para evitar que perform_race_reset() dispare inmediatamente.
        state_manager.get_pending_race_reset().store(false);
        // Asegurar que los checkpoints de la ronda actual estén cargados al iniciar
        setup_manager.load_checkpoints(current_round);
        RaceManager::reset_players_for_race_start(players, physics_config);
        npc_manager.reset_velocities();
    }

    // Cambiar el estado FUERA del lock para evitar deadlock con el game loop
    state_manager.transition_to_starting(10);
    started = true;
}

size_t GameLoop::get_player_count() const
{
    return player_manager.get_player_count();
}

bool GameLoop::has_player(int client_id) const
{
    return player_manager.has_player(client_id);
}

bool GameLoop::is_joinable() const
{
    return state_manager.is_joinable();
}

void GameLoop::add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox)
{
    player_manager.add_player(id, player_outbox, spawn_points);
}

void GameLoop::remove_player(int client_id)
{
    player_manager.remove_player(client_id, state_manager.get_state(), spawn_points);
}
