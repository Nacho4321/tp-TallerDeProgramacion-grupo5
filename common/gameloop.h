#ifndef GAMELOOP_H
#define GAMELOOP_H
#include "thread.h"
#include <unordered_map>
#include <array>
#include "../common/eventloop.h"
#include "PlayerData.h"
#include "../common/messages.h"
#include "../server/client_handler_msg.h"
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
    std::array<SpawnPoint, MAX_PLAYERS> spawn_points = {{
        {890.0f, 700.0f, 0.0f}, // Spawn 0
        {910.0f, 660.0f, 0.0f}, // Spawn 1
        {890.0f, 625.0f, 0.0f}, // Spawn 2
        {910.0f, 585.0f, 0.0f}, // Spawn 3
        {890.0f, 550.0f, 0.0f}, // Spawn 4
        {910.0f, 510.0f, 0.0f}, // Spawn 5
        {890.0f, 475.0f, 0.0f}, // Spawn 6
        {910.0f, 435.0f, 0.0f}  // Spawn 7
    }};
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
    };
    std::vector<MapLayout::WaypointData> street_waypoints; // grafo de waypoints para navegación de NPCs
    std::vector<NPCData> npcs;                             // lista activa de NPCs
    std::atomic<bool> reset_accumulator{false};            // flag para resetear acumulador de física al iniciar
    std::atomic<bool> pending_race_reset{false};           // flag para resetear la carrera fuera del callback de Box2D

    // Crea el cuerpo de un NPC. Recibe coordenadas ya en metros (el JSON se convierte a metros en MapLayout).
    b2Body *create_npc_body(float x_m, float y_m, bool is_static, float angle_rad = 0.0f);
    void init_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data); // spawn NPCs en el mapa
    void update_npcs();                                                       // avanzar movimiento de NPCs a lo largo de los waypoints

    CheckpointContactListener contact_listener;

    CarPhysicsConfig &physics_config;

    b2Body *create_player_body(float x, float y, Position &pos, const std::string &car_name);
    void broadcast_positions(ServerMessage &msg);
    void update_player_positions(std::vector<PlayerPositionUpdate> &broadcast);
    void update_body_positions();

    // Helpers usados por el contact listener
    int find_player_by_body(b2Body *body);
    void process_pair(b2Fixture *maybePlayerFix, b2Fixture *maybeCheckpointFix);

    b2Vec2 get_lateral_velocity(b2Body *body) const;
    b2Vec2 get_forward_velocity(b2Body *body) const;
    void update_friction_for_player(class PlayerData &player_data);
    void update_drive_for_player(class PlayerData &player_data);

    // Verifica si todos los jugadores terminaron la carrera (solo marca flag)
    void check_race_completion();
    // Ejecuta el reset al lobby cuando es seguro (fuera del callback de Box2D)
    void perform_race_reset();
    void update_bridge_state_for_player(PlayerData &player_data);

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