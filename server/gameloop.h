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
#include "gameloop/player/player_manager.h"
#include "gameloop/state/game_state_manager.h"
#include "gameloop/broadcast/broadcast_manager.h"
#include "gameloop/tick/tick_processor.h"
#include "gameloop/contact/contact_handler.h"
#include "gameloop/setup/setup_manager.h"
#define INITIAL_ID 1

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
    GameStateManager state_manager;
    int next_id;

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

    // ---------------- NPC Support ----------------
    NPCManager npc_manager;

    CarPhysicsConfig &physics_config;
    PlayerManager player_manager;
    BroadcastManager broadcast_manager;
    TickProcessor tick_processor;
    ContactHandler contact_handler;
    SetupManager setup_manager;

    // Ejecuta el reset al lobby cuando es seguro (fuera del callback de Box2D)
    void perform_race_reset();
    void advance_round_or_reset_to_lobby();



    // start_game helpers
    void on_playing_started();

public:
    explicit GameLoop(std::shared_ptr<Queue<Event>> events, uint8_t map_id = 0);
    void run() override;
    void start_game();
    void add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox);
    void remove_player(int client_id);
    bool has_player(int client_id) const;
    size_t get_player_count() const;
    bool is_joinable() const;
};
#endif