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
#define INITIAL_ID 1

enum class GameState
{
    LOBBY,  // Esperando que el host inicie el juego
    PLAYING // Juego en curso
};

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
    int next_id;

    // Spawn points para hasta 8 jugadores (en píxeles)
    struct SpawnPoint
    {
        float x;
        float y;
        float angle;
    };
    static constexpr int MAX_PLAYERS = 8;

    // Configuración de checkpoints y NPCs
    static constexpr int CHECKPOINT_LOOKAHEAD = 3;
    static constexpr float NPC_DIRECTION_THRESHOLD = 0.05f;
    static constexpr float NPC_ARRIVAL_THRESHOLD_M = 0.5f;
    static constexpr float MIN_DISTANCE_FROM_PARKED_M = 1.0f;

    // Vectores de dirección para Box2D
    static constexpr float RIGHT_VECTOR_X = 1.0f;
    static constexpr float RIGHT_VECTOR_Y = 0.0f;
    static constexpr float FORWARD_VECTOR_X = 0.0f;
    static constexpr float FORWARD_VECTOR_Y = 1.0f;
    
    // Spawn points loaded from JSON (pixels)
    std::vector<MapLayout::SpawnPointData> spawn_points;
    std::vector<int> player_order; // IDs de jugadores en orden de llegada

    MapLayout map_layout;
    // Mapa de fixtures de checkpoints a sus índices
    std::unordered_map<b2Fixture *, int> checkpoint_fixtures;
    // Centros de los checkpoints en metros del mundo, indexados por índice de checkpoint
    std::vector<b2Vec2> checkpoint_centers;

    // ---------------- NPC Support ----------------
    struct NPCData
    {
        b2Body *body{nullptr};
        int npc_id{0};             // id negativo para el broadcast
        int current_waypoint{0};   // último waypoint alcanzado
        int target_waypoint{0};    // próximo waypoint objetivo
        float speed_mps{0.0f};     // velocidad de movimiento en metros/segundo
        bool is_parked{false};     // true = estacionado (cuerpo estático)
        bool is_horizontal{false}; // true = orientado horizontalmente (solo para estacionados)
        bool on_bridge = false;
    };
    std::vector<MapLayout::WaypointData> street_waypoints; // grafo de waypoints para navegación de NPCs
    std::vector<NPCData> npcs;                             // lista activa de NPCs
    std::atomic<bool> reset_accumulator{false};            // flag para resetear acumulador de física al iniciar
    std::atomic<bool> pending_race_reset{false};           // flag para resetear la carrera fuera del callback de Box2D

    // Crea el cuerpo de un NPC. Recibe coordenadas ya en metros (el JSON se convierte a metros en MapLayout).
    b2Body *create_npc_body(float x_m, float y_m, bool is_static, float angle_rad = 0.0f);
    void init_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data);
    void spawn_parked_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data, int &next_negative_id);
    void spawn_moving_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data, int &next_negative_id);
    std::vector<int> get_valid_waypoints_away_from_parked(const std::vector<MapLayout::ParkedCarData> &parked_data);
    int select_closest_waypoint_connection(int start_waypoint_idx);
    float calculate_initial_npc_angle(const b2Vec2 &spawn_pos, const b2Vec2 &target_pos) const;
    NPCData create_moving_npc(int start_idx, int target_idx, float initial_angle, int &next_negative_id);
    void update_npcs();
    bool should_select_new_waypoint(NPCData &npc, const b2Vec2 &target_pos);
    void select_next_waypoint(NPCData &npc, std::mt19937 &gen);
    void move_npc_towards_target(NPCData &npc, const b2Vec2 &target_pos);

    CheckpointContactListener contact_listener;

    // Helper for picking nearest spawn point
    SpawnPoint pick_best_spawn(float x_px, float y_px) const;

    CarPhysicsConfig &physics_config;

    b2Body *create_player_body(float x, float y, Position &pos, const std::string &car_name);
    void broadcast_positions(ServerMessage &msg);
    void update_player_positions(std::vector<PlayerPositionUpdate> &broadcast);
    void add_player_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast, int player_id, PlayerData &player_data);
    void add_npc_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast, NPCData &npc);
    void update_body_positions();

    // Helpers usados por el contact listener
    int find_player_by_body(b2Body *body);
    void process_pair(b2Fixture *maybePlayerFix, b2Fixture *maybeCheckpointFix);
    bool is_valid_checkpoint_collision(b2Fixture *player_fixture, b2Fixture *checkpoint_fixture,
                                       int &out_player_id, int &out_checkpoint_index);
    void handle_checkpoint_reached(PlayerData &player_data, int player_id, int checkpoint_index);
    void complete_player_race(PlayerData &player_data, int player_id);
    
    // Car collision damage system
    void handle_car_collision(b2Fixture *fixture_a, b2Fixture *fixture_b);
    void apply_collision_damage(PlayerData &player_data, float impact_velocity, const std::string &car_name, float frontal_multiplier = 1.0f);

    // Setup and initialization helpers
    void setup_world();
    void setup_checkpoints_from_file(const std::string &json_path);
    void setup_npc_config();

    // Game tick processing
    void process_playing_state(float &acum);
    void process_lobby_state();

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
    bool update_bridge_state_for_player(PlayerData &player_data);
    void set_car_category(PlayerData &player_data, uint16 newCategory);
    void update_bridge_state_for_npc(NPCData &npc_data);
    void set_npc_category(NPCData &npc_data, uint16 newCategory);

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

    // perform_race_reset helpers
    bool should_reset_race() const;
    void broadcast_race_end_message();
    void reset_all_players_to_lobby();
    void transition_to_lobby_state();

public:
    explicit GameLoop(std::shared_ptr<Queue<Event>> events);
    void handle_begin_contact(b2Fixture *a, b2Fixture *b);
    void run() override;
    void start_game();
    void add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox);
    void remove_player(int client_id);
    size_t get_player_count() const;
};
#endif