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
#define INITIAL_ID 1
#include "game_state.h"

class GameLoop : public Thread
{
private:
    // Contact listener para detectar BeginContact entre jugadores y sensores de checkpoints.
    class CheckpointContactListener : public b2ContactListener
    {
    private:
        GameLoop *owner = nullptr;

    public:
        void set_owner(GameLoop *g);
        void BeginContact(b2Contact *contact) override;
    };

    b2World world{b2Vec2(0.0f, 0.0f)};
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

    CheckpointContactListener contact_listener;

    CarPhysicsConfig &physics_config;

    b2Body *create_player_body(float x, float y, Position &pos, const std::string &car_name);
    void broadcast_positions(ServerMessage &msg);
    void update_player_positions(std::vector<PlayerPositionUpdate> &broadcast);
    void add_player_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast, int player_id, PlayerData &player_data);
    void update_body_positions();

    // Helpers usados por el contact listener
    int find_player_by_body(b2Body *body);
    void process_pair(b2Fixture *maybePlayerFix, b2Fixture *maybeCheckpointFix);
    void complete_player_race(PlayerData &player_data);
    void disqualify_player(PlayerData &player_data, int player_id);

    // Car collision damage system
    void handle_car_collision(b2Fixture *fixture_a, b2Fixture *fixture_b);
    void apply_collision_damage(PlayerData &player_data, int player_id, float impact_velocity, const std::string &car_name, float frontal_multiplier = 1.0f);

    // Setup and initialization helpers
    void setup_world();
    void setup_npc_config();
    void setup_map_layout();

    // Game tick processing
    void process_playing_state(float &acum);
    void process_lobby_state();
    void process_starting_state();

    // Utility helpers
    float normalize_angle(double angle) const;
    void safe_destroy_body(b2Body *&body);

    b2Vec2 get_lateral_velocity(b2Body *body) const;
    b2Vec2 get_forward_velocity(b2Body *body) const;
    void update_friction_for_player(class PlayerData &player_data);
    void update_drive_for_player(class PlayerData &player_data);
    float calculate_desired_speed(bool want_up, bool want_down, const CarPhysics &car_physics) const;
    void apply_forward_drive_force(b2Body *body, float desired_speed, const CarPhysics &car_physics);
    void apply_steering_torque(b2Body *body, bool want_left, bool want_right, float torque);

    // Verifica si todos los jugadores terminaron la carrera (solo marca flag)
    void check_race_completion();
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
    void reset_players_for_race_start();
    void reset_npcs_velocities();
    void broadcast_game_started();
    void transition_to_starting_state(int countdown_seconds);
    void maybe_finish_starting_and_play();

    // perform_race_reset helpers
    bool should_reset_race() const;
    void check_round_timeout();
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