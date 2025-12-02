#ifndef GAMELOOP_H
#define GAMELOOP_H
#include "../common/thread.h"
#include <unordered_map>
#include <array>
#include <random>
#include "eventloop.h"
#include "PlayerData.h"
#include "../common/messages.h"
#include "client_handler_msg.h"
#include <box2d/b2_world.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_fixture.h>
#include "map_layout.h"
#include "car_physics_config.h"
#include <atomic>
#include <chrono>
#include "gameloop/npc/npc_manager.h"
#include "gameloop/bridge/bridge_handler.h"
#include "gameloop/checkpoint/checkpoint_handler.h"
#include "gameloop/physics/physics_handler.h"
#include "gameloop/collision/collision_handler.h"
#include "gameloop/race/race_manager.h"
#include "gameloop/world/world_manager.h"
#define INITIAL_ID 1
#include "game_state.h"

class GameLoop : public Thread
{
private:
    WorldManager world_manager;
    mutable std::mutex players_map_mutex;
    std::unordered_map<int, PlayerData> players;
    std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> players_messanger;
    std::shared_ptr<Queue<Event>> event_queue;
    EventLoop event_loop;
    bool started;
    GameState game_state; // Estado actual del juego (lobby o jugando)
    // Cuenta regresiva antes de iniciar la carrera
    std::chrono::steady_clock::time_point starting_deadline{};
    bool starting_active{false};
    int next_id;

    // Contador de tiempo de ronda (10 minutos de timeout)
    std::chrono::steady_clock::time_point round_start_time{};
    bool round_timeout_checked{false};

    // Spawn points para hasta 8 jugadores (en píxeles)
    struct SpawnPoint
    {
        float x;
        float y;
        float angle;
    };
    static constexpr int MAX_PLAYERS = 8;

    // Configuración de checkpoints
    static constexpr int CHECKPOINT_LOOKAHEAD = 3;

    // Vectores de dirección para Box2D
    static constexpr float RIGHT_VECTOR_X = 1.0f;
    static constexpr float RIGHT_VECTOR_Y = 0.0f;
    static constexpr float FORWARD_VECTOR_X = 0.0f;
    static constexpr float FORWARD_VECTOR_Y = 1.0f;

    // Spawn points loaded from JSON (pixels)
    std::vector<MapLayout::SpawnPointData> spawn_points;
    std::vector<int> player_order; // IDs de jugadores en orden de llegada

    uint8_t map_id{0}; // 0=LibertyCity, 1=SanAndreas, 2=ViceCity

    MapLayout map_layout;
    // Mapa de fixtures de checkpoints a sus índices
    std::unordered_map<b2Fixture *, int> checkpoint_fixtures;
    // Centros de los checkpoints en metros del mundo, indexados por índice de checkpoint
    std::vector<b2Vec2> checkpoint_centers;

    // ----- Multi-race support (3 carreras en mismo mapa con distintos recorridos) -----
    int current_round{0}; // 0..2
    // Archivos de recorridos - se inicializan según map_id
    std::array<std::string, 3> checkpoint_sets;
    void load_current_round_checkpoints();

    // ---------------- NPC Support ----------------
    NPCManager npc_manager;
    std::atomic<bool> reset_accumulator{false};  // flag para resetear acumulador de física al iniciar
    std::atomic<bool> pending_race_reset{false}; // flag para resetear la carrera fuera del callback de Box2D

    CarPhysicsConfig &physics_config;

    void broadcast_positions(ServerMessage &msg);
    void update_player_positions(std::vector<PlayerPositionUpdate> &broadcast);
    void add_player_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast, int player_id, PlayerData &player_data);
    void update_body_positions();

    // Helpers usados por el contact listener
    void process_pair(b2Fixture *maybePlayerFix, b2Fixture *maybeCheckpointFix);

    // Setup and initialization helpers
    void setup_world();
    void setup_npc_config();
    void setup_map_layout();

    // Game tick processing
    void process_playing_state(float &acum);
    void process_lobby_state();
    void process_starting_state();

    // Ejecuta el reset al lobby cuando es seguro (fuera del callback de Box2D)
    void perform_race_reset();
    void advance_round_or_reset_to_lobby();

    // add_player/remove_player helpers
    bool can_add_player() const;
    int add_player_to_order(int player_id);
    PlayerData create_default_player_data(int spawn_idx);
    void cleanup_player_data(int client_id);
    void remove_from_player_order(int client_id);
    void reposition_remaining_players();

    // start_game helpers
    void transition_to_playing_state();
    void reset_npcs_velocities();
    void broadcast_game_started();
    void transition_to_starting_state(int countdown_seconds);
    void maybe_finish_starting_and_play();

    // perform_race_reset helpers
    void broadcast_race_end_message();
    void reset_all_players_to_lobby();
    void transition_to_lobby_state();

public:
    explicit GameLoop(std::shared_ptr<Queue<Event>> events, uint8_t map_id = 0);
    void handle_begin_contact(b2Fixture *a, b2Fixture *b);
    void run() override;
    void start_game();
    void add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox);
    void remove_player(int client_id);
    bool has_player(int client_id) const;
    size_t get_player_count() const;
    bool is_joinable() const;
};
#endif