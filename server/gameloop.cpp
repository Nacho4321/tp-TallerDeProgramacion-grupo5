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

void GameLoop::setup_checkpoints_from_file(const std::string &json_path)
{
    std::vector<b2Vec2> checkpoints;
    map_layout.extract_checkpoints(json_path, checkpoints);

    if (checkpoints.empty())
        return;

    checkpoint_centers = checkpoints;
    for (size_t i = 0; i < checkpoint_centers.size(); ++i)
    {
        b2BodyDef bd;
        bd.type = b2_staticBody;
        bd.position = checkpoint_centers[i];
        b2Body *checkpoint_body = world.CreateBody(&bd);

        b2CircleShape shape;
        shape.m_p.Set(0.0f, 0.0f);
        shape.m_radius = CHECKPOINT_RADIUS_PX / SCALE;

        b2FixtureDef fd;
        fd.shape = &shape;
        fd.isSensor = true;

        b2Fixture *fixture = checkpoint_body->CreateFixture(&fd);
        checkpoint_fixtures[fixture] = static_cast<int>(i);
    }

    std::cout << "[GameLoop] Created " << checkpoint_centers.size()
              << " checkpoint sensors (from " << json_path << ")." << std::endl;
}


void GameLoop::setup_npc_config()
{
    auto &npc_cfg = NPCConfig::getInstance();
    npc_cfg.loadFromFile("config/npc.yaml");
}

void GameLoop::setup_world()
{
    std::vector<MapLayout::ParkedCarData> parked_data;
    setup_map_layout();
    load_current_round_checkpoints();
    setup_npc_config();

    uint8_t safe_map_id = (map_id < MAP_COUNT) ? map_id : 0;
    map_layout.extract_map_npc_data(MAP_JSON_PATHS[safe_map_id], street_waypoints, parked_data);
    if (int(parked_data.size()) > 0)
    {
        init_npcs(parked_data);
    }
}

void GameLoop::setup_map_layout()
{
    uint8_t safe_map_id = (map_id < MAP_COUNT) ? map_id : 0;
    map_layout.create_map_layout(MAP_JSON_PATHS[safe_map_id]);
}

void GameLoop::load_current_round_checkpoints()
{
    // Limpiar checkpoints previos y fixtures
    for (auto &pair : checkpoint_fixtures)
    {
        b2Body *body = pair.first->GetBody();
        if (body)
        {
            world.DestroyBody(body);
        }
    }
    checkpoint_fixtures.clear();
    checkpoint_centers.clear();

    std::string json_path = checkpoint_sets[current_round];
    setup_checkpoints_from_file(json_path);
    std::cout << "[GameLoop] Round " << current_round+1 << " loaded checkpoints from: " << json_path << std::endl;
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

    // Reset collision flags at the start of each frame
    for (auto &entry : players)
    {
        entry.second.collision_this_frame = false;
    }

    update_npcs();
    update_body_positions();

    while (acum >= FPS)
    {
        world.Step(FPS, VELOCITY_ITERS, COLLISION_ITERS);
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
                complete_player_race(player_data, id);
                player_data.pending_race_complete = false;
            }
            
            // Procesar cheat de descalificación pendiente
            if (player_data.pending_disqualification && !player_data.is_dead)
            {
                disqualify_player(player_data, id);
                player_data.pending_disqualification = false;
            }
            
            if (player_data.mark_body_for_removal && player_data.body)
            {
                if (world.IsLocked())
                {
                    // Esto no debería pasar acá, pero dejo el log por si aparece alguna condición rara.
                    std::cout << "[GameLoop] World locked during flush, postergando destroy para player " << id << std::endl;
                    continue;
                }
                safe_destroy_body(player_data.body);
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

    std::vector<PlayerPositionUpdate> broadcast;
    update_player_positions(broadcast);

    ServerMessage msg;
    msg.opcode = UPDATE_POSITIONS;
    msg.positions = broadcast;
    broadcast_positions(msg);
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

// Constructor para poder setear el contact listener del world
GameLoop::GameLoop(std::shared_ptr<Queue<Event>> events, uint8_t map_id_param)
    : players_map_mutex(), players(), players_messanger(), event_queue(events), event_loop(players_map_mutex, players, event_queue), started(false), game_state(GameState::LOBBY), next_id(INITIAL_ID), map_id(map_id_param), map_layout(world), physics_config(CarPhysicsConfig::getInstance())
{
    if (!physics_config.loadFromFile("config/car_physics.yaml"))
    {
        std::cerr << "[GameLoop] WARNING: Failed to load car physics config, using defaults" << std::endl;
    }

    // Inicializar rutas según el mapa seleccionado
    uint8_t safe_map_id = (map_id < MAP_COUNT) ? map_id : 0;
    for (int i = 0; i < 3; ++i) {
        checkpoint_sets[i] = MAP_CHECKPOINT_PATHS[safe_map_id][i];
    }
    
    // Cargar spawn points del mapa correspondiente
    map_layout.extract_spawn_points(MAP_SPAWN_POINTS_PATHS[safe_map_id], spawn_points);
    
    std::cout << "[GameLoop] Initialized with map_id=" << int(map_id) 
              << " (" << MAP_NAMES[safe_map_id] << ")" << std::endl;

    // seteo el contact listener owner y lo registro con el world
    contact_listener.set_owner(this);
    world.SetContactListener(&contact_listener);
}

void GameLoop::CheckpointContactListener::BeginContact(b2Contact *contact)
{
    if (!owner)
        return;
    b2Fixture *fixture_a = contact->GetFixtureA();
    b2Fixture *fixture_b = contact->GetFixtureB();
    owner->handle_begin_contact(fixture_a, fixture_b);
}

void GameLoop::CheckpointContactListener::set_owner(GameLoop *g)
{
    owner = g;
}

int GameLoop::find_player_by_body(b2Body *body)
{
    for (auto &entry : players)
    {
        if (entry.second.body == body)
            return entry.first; // player id
    }
    return -1;
}

float GameLoop::normalize_angle(double angle) const
{
    while (angle < 0.0)
        angle += 2.0 * M_PI;
    while (angle >= 2.0 * M_PI)
        angle -= 2.0 * M_PI;
    return static_cast<float>(angle);
}

void GameLoop::safe_destroy_body(b2Body *&body)
{
    if (!body)
        return;

    b2World *w = body->GetWorld();
    if (!w)
        return;

    try
    {
        w->DestroyBody(body);
        body = nullptr;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[GameLoop] Error destroying body: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "[GameLoop] Unknown error destroying body" << std::endl;
    }
}

b2Vec2 GameLoop::get_lateral_velocity(b2Body *body) const
{
    b2Vec2 currentRightNormal = body->GetWorldVector(b2Vec2(RIGHT_VECTOR_X, RIGHT_VECTOR_Y));
    return b2Dot(currentRightNormal, body->GetLinearVelocity()) * currentRightNormal;
}

b2Vec2 GameLoop::get_forward_velocity(b2Body *body) const
{
    b2Vec2 currentForwardNormal = body->GetWorldVector(b2Vec2(FORWARD_VECTOR_X, FORWARD_VECTOR_Y));
    return b2Dot(currentForwardNormal, body->GetLinearVelocity()) * currentForwardNormal;
}

void GameLoop::update_friction_for_player(PlayerData &player_data)
{
    b2Body *body = player_data.body;
    if (!body)
        return;

    const CarPhysics &car_physics = physics_config.getCarPhysics(player_data.car.car_name);

    // impulso lateral para reducir el deslizamiento lateral (limitado para permitir derrapes)
    b2Vec2 impulse = body->GetMass() * -get_lateral_velocity(body);
    float ilen = impulse.Length();
    float maxImpulse = car_physics.max_lateral_impulse * body->GetMass();
    if (ilen > maxImpulse)
        impulse *= maxImpulse / ilen;
    body->ApplyLinearImpulse(impulse, body->GetWorldCenter(), true);

    // matar un poco la velocidad angular para evitar giros descontrolados
    body->ApplyAngularImpulse(car_physics.angular_friction * body->GetInertia() * -body->GetAngularVelocity(), true);

    b2Vec2 forwardDir = body->GetWorldVector(b2Vec2(0, 1));
    float currentForwardSpeed = b2Dot(body->GetLinearVelocity(), forwardDir);
    b2Vec2 dragForce = body->GetMass() * car_physics.forward_drag_coefficient * currentForwardSpeed * forwardDir;
    body->ApplyForce(dragForce, body->GetWorldCenter(), true);
}

float GameLoop::calculate_desired_speed(bool want_up, bool want_down, const CarPhysics &car_physics) const
{
    if (want_up)
        return car_physics.max_speed / SCALE;
    if (want_down)
        return -car_physics.max_speed * car_physics.backward_speed_multiplier / SCALE;
    return 0.0f;
}

void GameLoop::apply_forward_drive_force(b2Body *body, float desired_speed, const CarPhysics &car_physics)
{
    b2Vec2 forwardNormal = body->GetWorldVector(b2Vec2(FORWARD_VECTOR_X, FORWARD_VECTOR_Y));
    float current_speed = b2Dot(body->GetLinearVelocity(), forwardNormal);

    float max_accel_m = car_physics.max_acceleration / SCALE;
    if (desired_speed < 0.0f)
    {
        max_accel_m *= car_physics.backward_speed_multiplier;
    }
    float accel_command = (desired_speed - current_speed) * car_physics.speed_controller_gain;

    if (accel_command > max_accel_m)
        accel_command = max_accel_m;
    else if (accel_command < -max_accel_m)
        accel_command = -max_accel_m;

    float desired_force = body->GetMass() * accel_command;
    body->ApplyForce(desired_force * forwardNormal, body->GetWorldCenter(), true);
}

void GameLoop::apply_steering_torque(b2Body *body, bool want_left, bool want_right, float torque)
{
    if (want_left)
        body->ApplyTorque(-torque, true);
    else if (want_right)
        body->ApplyTorque(torque, true);
}

void GameLoop::update_drive_for_player(PlayerData &player_data)
{
    b2Body *body = player_data.body;
    if (!body)
        return;

    CarPhysics car_physics = physics_config.getCarPhysics(player_data.car.car_name);
    // Sobreescribir con valores upgradeados del player
    car_physics.max_speed = player_data.car.speed;
    car_physics.max_acceleration = player_data.car.acceleration;
    car_physics.torque = player_data.car.handling;

    bool want_up = (player_data.position.direction_y == up);
    bool want_down = (player_data.position.direction_y == down);
    bool want_left = (player_data.position.direction_x == left);
    bool want_right = (player_data.position.direction_x == right);


    // Detectar frenazo
    player_data.is_stopping = false;
    if (want_down) {
        b2Vec2 forwardNormal = body->GetWorldVector(b2Vec2(FORWARD_VECTOR_X, FORWARD_VECTOR_Y));
        float current_speed = b2Dot(body->GetLinearVelocity(), forwardNormal);
        float threshold = 1.0f; // habria q ver el threshold 
        if (current_speed > threshold) {
            player_data.is_stopping = true;
            std::cout << "[FRENAZO] Player " << " frenazo Velocidad: " << current_speed << std::endl;
        }
    }

    if (want_up || want_down)
    {
        float desired_speed = calculate_desired_speed(want_up, want_down, car_physics);
        apply_forward_drive_force(body, desired_speed, car_physics);
    }

    apply_steering_torque(body, want_left, want_right, car_physics.torque);
}

bool GameLoop::is_valid_checkpoint_collision(b2Fixture *player_fixture, b2Fixture *checkpoint_fixture,
                                             int &out_player_id, int &out_checkpoint_index)
{
    if (!player_fixture || !checkpoint_fixture)
        return false;

    b2Body *player_body = player_fixture->GetBody();
    out_player_id = find_player_by_body(player_body);
    if (out_player_id < 0)
        return false;

    auto it = checkpoint_fixtures.find(checkpoint_fixture);
    if (it == checkpoint_fixtures.end())
        return false;

    out_checkpoint_index = it->second;
    return true;
}

void GameLoop::complete_player_race(PlayerData &player_data, int player_id)
{
    auto lap_end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(lap_end_time - player_data.lap_start_time).count();

    player_data.race_finished = true;
    player_data.disqualified = false;

    // Registrar tiempo de la ronda actual (limitar a 3 rondas)
    if (static_cast<int>(player_data.round_times_ms.size()) < TOTAL_ROUNDS) {
        player_data.round_times_ms.push_back(static_cast<uint32_t>(elapsed_ms));
    } else {
        player_data.round_times_ms[player_data.rounds_completed % TOTAL_ROUNDS] = static_cast<uint32_t>(elapsed_ms);
    }
    player_data.rounds_completed = std::min(player_data.rounds_completed + 1, TOTAL_ROUNDS);
    player_data.total_time_ms += static_cast<uint32_t>(elapsed_ms);
    
    // Activar inmortalidad al terminar la ronda (para que no muera mientras espera)
    player_data.god_mode = true;

    int minutes = static_cast<int>(elapsed_ms / 60000);
    float seconds = static_cast<float>((elapsed_ms % 60000)) / 1000.0f;

    std::cout << "\n========================================" << std::endl;
    std::cout << "   PLAYER " << player_id << " FINISHED THE RACE!" << std::endl;
    std::cout << "   Time: " << minutes << ":" << std::fixed << std::setprecision(3) << seconds << std::endl;
    std::cout << "   Waiting for other players... (GOD MODE ON)" << std::endl;
    std::cout << "========================================\n"
              << std::endl;

    check_race_completion();
}

void GameLoop::disqualify_player(PlayerData &player_data, int player_id)
{
    // Marcar como muerto y descalificado
    player_data.car.hp = 0.0f;
    player_data.is_dead = true;
    player_data.race_finished = true;
    player_data.disqualified = true;
    player_data.god_mode = false;  // Desactivar god mode
    
    // Asignar tiempo de descalificación: 10 minutos
    uint32_t dq_ms = 10u * 60u * 1000u;
    if (static_cast<int>(player_data.round_times_ms.size()) < TOTAL_ROUNDS) {
        player_data.round_times_ms.push_back(dq_ms);
    } else {
        player_data.round_times_ms[player_data.rounds_completed % TOTAL_ROUNDS] = dq_ms;
    }
    player_data.rounds_completed = std::min(player_data.rounds_completed + 1, TOTAL_ROUNDS);
    player_data.total_time_ms += dq_ms;
    
    // Marcar el body para destrucción
    player_data.mark_body_for_removal = true;
    player_data.collision_this_frame = true;
    
    std::cout << "[Death] Player " << player_id << " DISQUALIFIED! (HP=0, 10min penalty)" << std::endl;
    
    check_race_completion();
}

void GameLoop::handle_checkpoint_reached(PlayerData &player_data, int player_id, int checkpoint_index)
{
    if (player_data.race_finished)
        return;

    if (player_data.next_checkpoint != checkpoint_index)
        return;

    int total = static_cast<int>(checkpoint_centers.size());
    int new_next = player_data.next_checkpoint + 1;

    if (total > 0 && new_next >= total)
    {
        complete_player_race(player_data, player_id);
    }
    else
    {
        player_data.next_checkpoint = new_next;
        
        std::cout << "[GameLoop] Player " << player_id << " passed checkpoint " 
                  << checkpoint_index << " next=" << player_data.next_checkpoint << std::endl;
    }
}

void GameLoop::process_pair(b2Fixture *maybePlayerFix, b2Fixture *maybeCheckpointFix)
{
    int player_id, checkpoint_index;
    if (!is_valid_checkpoint_collision(maybePlayerFix, maybeCheckpointFix, player_id, checkpoint_index))
        return;

    PlayerData &player_data = players[player_id];
    handle_checkpoint_reached(player_data, player_id, checkpoint_index);
}

void GameLoop::handle_begin_contact(b2Fixture *fixture_a, b2Fixture *fixture_b)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    
    // Checkeo checkpoint
    process_pair(fixture_a, fixture_b);
    process_pair(fixture_b, fixture_a);
    
    // Checkeo colisiones entre autos
    handle_car_collision(fixture_a, fixture_b);
}

void GameLoop::handle_car_collision(b2Fixture *fixture_a, b2Fixture *fixture_b)
{
    if (game_state != GameState::PLAYING)
        return;

    b2Body *body_a = fixture_a->GetBody();
    b2Body *body_b = fixture_b->GetBody();

    uint16 cat_a = fixture_a->GetFilterData().categoryBits;
    uint16 cat_b = fixture_b->GetFilterData().categoryBits;

    // Skipeo si alguno es sensor 
    bool a_is_sensor = fixture_a->IsSensor();
    bool b_is_sensor = fixture_b->IsSensor();
    
    if (a_is_sensor || b_is_sensor)
        return; 

    bool a_is_player = (cat_a == CAR_GROUND || cat_a == CAR_BRIDGE);
    bool b_is_player = (cat_b == CAR_GROUND || cat_b == CAR_BRIDGE);

    // TODO: borrar este log cuando funcione todo bien
    std::string collision_type_a = "";
    std::string collision_type_b = "";
    
    if (a_is_player && b_is_player) {
        collision_type_a = "PLAYER vs PLAYER";
        collision_type_b = "PLAYER vs PLAYER";
    } else if (a_is_player) {
        if (cat_b == 0x0001) {
            collision_type_a = "PLAYER vs WALL";
        } else {
            collision_type_a = "PLAYER vs NPC/OBSTACLE";
        }
    } else if (b_is_player) {
        if (cat_a == 0x0001) {
            collision_type_b = "PLAYER vs WALL";
        } else {
            collision_type_b = "PLAYER vs NPC/OBSTACLE";
        }
    }

    // Aplicar daño para el jugador A si es un jugador
    if (a_is_player)
    {
        int player_a_id = find_player_by_body(body_a);
        if (player_a_id != -1)
        {
            auto it_a = players.find(player_a_id);
            if (it_a != players.end())
            {
                b2Vec2 vel_a = body_a->GetLinearVelocity();
                b2Vec2 vel_b = body_b->GetLinearVelocity();
                b2Vec2 relative_vel = vel_a - vel_b;
                float impact_velocity = relative_vel.Length();

                // Detectar choque frontal: si las velocidades son opuestas (ángulo cercano a 180°)
                float frontal_multiplier = 1.0f;
                if (vel_a.Length() > 1.0f && vel_b.Length() > 1.0f) {
                    // Normalizar y calcular producto punto
                    b2Vec2 dir_a = vel_a;
                    dir_a.Normalize();
                    b2Vec2 dir_b = vel_b;
                    dir_b.Normalize();
                    float dot = b2Dot(dir_a, dir_b);
                    
                    // Si dot < -0.5, significa que los vectores apuntan en direcciones opuestas
                    // (ángulo > 120°). Cuanto más negativo, más frontal es el choque.
                    if (dot < -0.5f) {
                        // Mapear de [-1.0, -0.5] a [2.5, 1.5] 
                        // dot = -1.0 (180°) -> multiplier = 2.5x
                        // dot = -0.5 (120°) -> multiplier = 1.5x
                        frontal_multiplier = 1.5f + (-dot - 0.5f) * 2.0f;
                    }
                }

                std::cout << "[Collision] " << collision_type_a 
                          << " | Player " << player_a_id 
                          << " | Impact: " << impact_velocity << " m/s" << std::endl;

                apply_collision_damage(it_a->second, player_a_id, impact_velocity, it_a->second.car.car_name, frontal_multiplier);
            }
        }
    }

    // Aplicar daño para el jugador B si es un jugador
    if (b_is_player)
    {
        int player_b_id = find_player_by_body(body_b);
        if (player_b_id != -1)
        {
            auto it_b = players.find(player_b_id);
            if (it_b != players.end())
            {
                b2Vec2 vel_a = body_a->GetLinearVelocity();
                b2Vec2 vel_b = body_b->GetLinearVelocity();
                b2Vec2 relative_vel = vel_b - vel_a;  // Invertido para B
                float impact_velocity = relative_vel.Length();

                // Detectar choque frontal para el jugador B
                float frontal_multiplier = 1.0f;
                if (vel_a.Length() > 1.0f && vel_b.Length() > 1.0f) {
                    // Agarro los 2 vectores de velocidad, los normalizo y calculo el producto punto
                    b2Vec2 dir_a = vel_a;
                    dir_a.Normalize();
                    b2Vec2 dir_b = vel_b;
                    dir_b.Normalize();
                    float dot = b2Dot(dir_a, dir_b);
                    
                    if (dot < -0.5f) {
                        frontal_multiplier = 1.5f + (-dot - 0.5f) * 2.0f;
                    }
                }

                if (!a_is_player || !b_is_player) {
                    std::cout << "[Collision] " << collision_type_b 
                              << " | Player " << player_b_id 
                              << " | Impact: " << impact_velocity << " m/s" << std::endl;
                }

                apply_collision_damage(it_b->second, player_b_id, impact_velocity, it_b->second.car.car_name, frontal_multiplier);
            }
        }
    }
}

void GameLoop::apply_collision_damage(PlayerData &player_data, int player_id, float impact_velocity, const std::string &car_name, float frontal_multiplier)
{
    if (player_data.is_dead)
    {
        std::cout << "[Collision] Player already dead, skipping damage" << std::endl;
        return;
    }

    // Usar durability del player (upgradeada) en lugar del YAML
    float durability = player_data.car.durability;

    // Formula: damage = impact_velocity * multiplier * frontal_multiplier * 0.1 
    // frontal_multiplier: 1.0 para colisiones normales, 2.5 para choques frontales en contramano
    // (10 m/s) son aprox 10 damage (o 25 si es frontal)
    float damage = impact_velocity * durability * frontal_multiplier * 0.1f;

    // Solo aplico daño si es significativo
    // TODO: Por ahi meter el 0.5 al YAML o hacerlo una constante global
    if (damage < 0.5f)
    {
        std::cout << "[Collision] Impact too weak (" << impact_velocity 
                  << " m/s), damage=" << damage << " < 0.5, skipping" << std::endl;
        return;
    }

    // God mode: no recibe daño
    if (player_data.god_mode)
    {
        std::cout << "[Collision] Player has GOD MODE - damage ignored (" << damage << ")" << std::endl;
        return;
    }

    player_data.car.hp -= damage;

    player_data.collision_this_frame = true;

    if (frontal_multiplier > 1.5f) {
        std::cout << "[Collision] **FRONTAL HEAD-ON CRASH** Player car '" << car_name 
                  << "' took " << damage << " damage (impact: " << impact_velocity 
                  << " m/s, frontal multiplier: " << frontal_multiplier 
                  << "). HP remaining: " << player_data.car.hp << std::endl;
    } else {
        std::cout << "[Collision] Player car '" << car_name 
                  << "' took " << damage << " damage (impact: " << impact_velocity 
                  << " m/s). HP remaining: " << player_data.car.hp << std::endl;
    }

    if (player_data.car.hp <= 0.0f)
    {
        disqualify_player(player_data, player_id);
    }
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

PlayerData GameLoop::create_default_player_data(int spawn_idx)
{
    const MapLayout::SpawnPointData &spawn = spawn_points[spawn_idx];
    std::cout << "[GameLoop] add_player: assigning spawn point " << spawn_idx
              << " at (" << spawn.x << "," << spawn.y << ")" << std::endl;

    const CarPhysics &car_phys = physics_config.getCarPhysics(GREEN_CAR);

    Position pos = Position{false, spawn.x, spawn.y, not_horizontal, not_vertical, spawn.angle};
    PlayerData player_data;
    player_data.body = create_player_body(spawn.x, spawn.y, pos, GREEN_CAR);
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
    safe_destroy_body(pd.body);

    players_messanger.erase(client_id);
    players.erase(it);
}

void GameLoop::remove_from_player_order(int client_id)
{
    auto order_it = std::find(player_order.begin(), player_order.end(), client_id);
    if (order_it != player_order.end())
        player_order.erase(order_it);
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

        safe_destroy_body(player_data.body);
        Position new_pos{false, spawn.x, spawn.y, not_horizontal, not_vertical, spawn.angle};
        player_data.body = create_player_body(spawn.x, spawn.y, new_pos, player_data.car.car_name);
        player_data.position = new_pos;
    }
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

void GameLoop::transition_to_playing_state()
{
    game_state = GameState::PLAYING;
    std::cout << "[GameLoop] Game started! Transitioning from LOBBY to PLAYING" << std::endl;
    reset_accumulator.store(true);
    
    broadcast_game_started();
}

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
            catch (const ClosedQueue &) {}
        }
    }
}

void GameLoop::reset_players_for_race_start()
{
    auto race_start_time = std::chrono::steady_clock::now();

    for (auto &[id, player_data] : players)
    {
        if (!player_data.body)
            continue;

        player_data.body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
        player_data.body->SetAngularVelocity(0.0f);

        player_data.lap_start_time = race_start_time;
        player_data.next_checkpoint = 0;
        player_data.race_finished = false;
        player_data.is_dead = false;
        player_data.god_mode = false;  // Reset god mode para nueva ronda
        player_data.position.on_bridge = false;
        player_data.position.direction_x = not_horizontal;
        player_data.position.direction_y = not_vertical;
        
        // Reset HP 
        const CarPhysics &car_physics = physics_config.getCarPhysics(player_data.car.car_name);
        player_data.car.hp = car_physics.max_hp;

        b2Vec2 current_pos = player_data.body->GetPosition();
        float current_x_px = current_pos.x * SCALE;
        float current_y_px = current_pos.y * SCALE;

        std::cout << "[GameLoop] start_game: Player " << id
                  << " at body_pos=(" << current_x_px << "," << current_y_px << ")"
                  << " stored_pos=(" << player_data.position.new_X << "," << player_data.position.new_Y << ")"
                  << " HP=" << player_data.car.hp
                  << " - race timer started"
                  << std::endl;
    }
}

void GameLoop::reset_npcs_velocities()
{
    for (auto &npc : npcs)
    {
        if (!npc.body || npc.is_parked)
            continue;
        npc.body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
    }
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
        reset_players_for_race_start();
        reset_npcs_velocities();
    } // <-- Lock se libera aquí

    // Cambiar el estado FUERA del lock para evitar deadlock con el game loop
    transition_to_starting_state(10);
    started = true;
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

bool GameLoop::should_reset_race() const
{
    return pending_race_reset.load();
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
            if (!pd.round_times_ms.empty()) {
                time_ms = pd.round_times_ms.back();
            }
            msg.race_times.push_back({static_cast<uint32_t>(pid), time_ms, dq, round_index});
        }
    }
    broadcast_positions(msg);
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

        safe_destroy_body(player_data.body);
        Position new_pos{false, spawn.x, spawn.y, not_horizontal, not_vertical, spawn.angle};
        player_data.body = create_player_body(spawn.x, spawn.y, new_pos, player_data.car.car_name);
        player_data.position = new_pos;

        player_data.next_checkpoint = 0;
        player_data.race_finished = false;
        player_data.is_dead = false;
        player_data.god_mode = false;  // Reset god mode para nueva ronda
        player_data.lap_start_time = std::chrono::steady_clock::now();
        
        // Reseteo HP
        const CarPhysics &car_physics = physics_config.getCarPhysics(player_data.car.car_name);
        player_data.car.hp = car_physics.max_hp;
    }
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

b2Body *GameLoop::create_player_body(float x_px, float y_px, Position &pos, const std::string &car_name)
{
    const CarPhysics &car_physics = physics_config.getCarPhysics(car_name);

    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position.Set(x_px / SCALE, y_px / SCALE);
    bd.angle = pos.angle;

    b2Body *player_body = world.CreateBody(&bd);

    float halfWidth = car_physics.width / (2.0f * SCALE);
    float halfHeight = car_physics.height / (2.0f * SCALE);
    b2Vec2 center_offset(0.0f, car_physics.center_offset_y / SCALE);

    b2PolygonShape shape;
    shape.SetAsBox(halfWidth, halfHeight, center_offset, 0.0f);

    b2FixtureDef fd;
    fd.shape = &shape;
    fd.density = car_physics.density;
    fd.friction = car_physics.friction;
    fd.restitution = car_physics.restitution;

    fd.filter.categoryBits = CAR_GROUND;

    fd.filter.maskBits =
        COLLISION_FLOOR |     // Colisiones del suelo
        CAR_GROUND |          // Otros jugadores en el suelo
        SENSOR_START_BRIDGE | // Sensores de entrada
        SENSOR_END_BRIDGE;    // Sensores de salida

    player_body->CreateFixture(&fd);
    player_body->SetBullet(true);
    player_body->SetLinearDamping(car_physics.linear_damping);
    player_body->SetAngularDamping(car_physics.angular_damping);

    return player_body;
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
        player_data.position.angle = normalize_angle(body->GetAngle());

        // IMPORTANTE: Actualizar el estado del puente ANTES de copiar la posición
        update_bridge_state_for_player(player_data);
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

void GameLoop::add_npc_to_broadcast(std::vector<PlayerPositionUpdate> &broadcast, NPCData &npc)
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
    update_bridge_state_for_npc(npc);
    pos.on_bridge = npc.on_bridge;
    PlayerPositionUpdate update;
    update.player_id = npc.npc_id; // id negativo para NPC
    update.new_pos = pos;
    update.car_type = "npc";
    update.hp = 100.0f; 
    update.collision_flag = false; 
    broadcast.push_back(update);
}

void GameLoop::update_player_positions(std::vector<PlayerPositionUpdate> &broadcast)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);

    for (auto &[id, player_data] : players)
    {
        add_player_to_broadcast(broadcast, id, player_data);
    }

    for (auto &npc : npcs)
    {
        add_npc_to_broadcast(broadcast, npc);
    }
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
        update_friction_for_player(player_data);

        // Aplicar fuerza de conducción / torque basado en la entrada
        update_drive_for_player(player_data);
    }
}

// ---------------- NPC Implementation ----------------

b2Body *GameLoop::create_npc_body(float x_m, float y_m, bool is_static, float angle_rad)
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

void GameLoop::spawn_parked_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data, int &next_negative_id)
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

    std::cout << "[GameLoop][NPC] Spawned " << parked_count << " parked NPCs (out of "
              << parked_data.size() << " available positions)." << std::endl;
}

std::vector<int> GameLoop::get_valid_waypoints_away_from_parked(const std::vector<MapLayout::ParkedCarData> &parked_data)
{
    std::vector<int> candidate_waypoints;
    candidate_waypoints.reserve(street_waypoints.size());

    for (int idx = 0; idx < static_cast<int>(street_waypoints.size()); ++idx)
    {
        b2Vec2 wp_pos = street_waypoints[idx].position;
        bool too_close = false;

        for (const auto &parked_car : parked_data)
        {
            float distance = (wp_pos - parked_car.position).Length();
            if (distance < MIN_DISTANCE_FROM_PARKED_M)
            {
                too_close = true;
                break;
            }
        }

        if (!too_close)
            candidate_waypoints.push_back(idx);
    }

    if (candidate_waypoints.empty())
    {
        for (int idx = 0; idx < static_cast<int>(street_waypoints.size()); ++idx)
            candidate_waypoints.push_back(idx);
        std::cout << "[GameLoop][NPC] Warning: All waypoints filtered out by parked proximity; using full set." << std::endl;
    }

    return candidate_waypoints;
}

int GameLoop::select_closest_waypoint_connection(int start_waypoint_idx)
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

float GameLoop::calculate_initial_npc_angle(const b2Vec2 &spawn_pos, const b2Vec2 &target_pos) const
{
    b2Vec2 direction = target_pos - spawn_pos;
    float length = direction.Length();

    if (length <= 0.0001f)
        return 0.0f;

    float movement_angle = std::atan2(direction.y, direction.x);
    return movement_angle - b2_pi / 2.0f; // sprite orientado hacia arriba
}

GameLoop::NPCData GameLoop::create_moving_npc(int start_idx, int target_idx, float initial_angle, int &next_negative_id)
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

void GameLoop::spawn_moving_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data, int &next_negative_id)
{
    if (street_waypoints.size() < 2)
    {
        std::cout << "[GameLoop][NPC] Not enough waypoints to spawn moving NPCs." << std::endl;
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

    std::cout << "[GameLoop][NPC] Moving NPC spawn candidates: " << candidate_waypoints.size()
              << " chosen: " << moving_npcs_count << std::endl;
    std::cout << "[GameLoop][NPC] Spawned " << moving_npcs_count << " moving NPCs." << std::endl;
    std::cout << "[GameLoop][NPC] Total NPCs: " << npcs.size() << std::endl;
}

void GameLoop::init_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    int next_negative_id = -1;
    spawn_parked_npcs(parked_data, next_negative_id);
    spawn_moving_npcs(parked_data, next_negative_id);
}

bool GameLoop::should_select_new_waypoint(NPCData &npc, const b2Vec2 &target_pos)
{
    b2Vec2 pos = npc.body->GetPosition();
    float dist = (target_pos - pos).Length();
    return dist < NPC_ARRIVAL_THRESHOLD_M;
}

void GameLoop::select_next_waypoint(NPCData &npc, std::mt19937 &gen)
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

void GameLoop::move_npc_towards_target(NPCData &npc, const b2Vec2 &target_pos)
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

void GameLoop::update_npcs()
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
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

size_t GameLoop::get_player_count() const
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    return players.size();
}

bool GameLoop::is_joinable() const
{
    return game_state == GameState::LOBBY;
}

void GameLoop::check_race_completion()
{
    // Asume que ya estamos bajo players_map_mutex lock (llamado desde process_pair o apply_collision_damage)
    // IMPORTANTE: No podemos modificar Box2D aca (estamos en callback de contacto)

    int finished_or_dead_count = 0;
    int total_players = static_cast<int>(players.size());

    for (const auto &[id, player_data] : players)
    {
        // Contar jugadores que terminaron la carrera O murieron
        if (player_data.race_finished || player_data.is_dead)
        {
            finished_or_dead_count++;
        }
    }

    // La carrera termina cuando TODOS los jugadores terminaron O murieron
    bool all_done = (finished_or_dead_count == total_players && total_players > 0);

    if (all_done)
    {
        std::cout << "\n======================================" << std::endl;
        std::cout << "   ALL PLAYERS FINISHED OR DIED!" << std::endl;
        std::cout << "   Preparing to return to lobby..." << std::endl;
        std::cout << "======================================\n"
                  << std::endl;

        pending_race_reset.store(true);
    }
}

void GameLoop::perform_race_reset()
{
    // Evitar realizar operaciones pesadas (destroy/recreate bodies, reload checkpoints)
    // bajo el lock del mapa de jugadores. Primero chequeamos el flag y salimos del lock.
    bool do_reset = false;
    {
        std::lock_guard<std::mutex> lk(players_map_mutex);
        if (should_reset_race())
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

bool GameLoop::update_bridge_state_for_player(PlayerData &player_data)
{
    if (!player_data.body)
        return player_data.position.on_bridge;
    if (!player_data.body)
    {
        return false;
    }

    bool entering_bridge = false;
    bool leaving_bridge = false;

    b2ContactEdge *ce = player_data.body->GetContactList();
    while (ce)
    {
        b2Contact *c = ce->contact;

        if (c->IsTouching())
        {
            b2Fixture *a = c->GetFixtureA();
            b2Fixture *b = c->GetFixtureB();

            uint16 fcA = a->GetFilterData().categoryBits;
            uint16 fcB = b->GetFilterData().categoryBits;

            // Detectar entrada al puente
            if (fcA == SENSOR_START_BRIDGE || fcB == SENSOR_START_BRIDGE)
            {
                entering_bridge = true;
            }

            // Detectar salida del puente
            if (fcA == SENSOR_END_BRIDGE || fcB == SENSOR_END_BRIDGE)
            {
                leaving_bridge = true;
            }
        }

        ce = ce->next;
    }

    // Actualizar el estado del puente
    if (entering_bridge && !player_data.position.on_bridge)
    {
        // Entrando al puente
        std::cout << "[BRIDGE] Player entering bridge (CAR_GROUND -> CAR_BRIDGE)" << std::endl;
        set_car_category(player_data, CAR_BRIDGE);
        player_data.position.on_bridge = true;
    }
    else if (leaving_bridge && player_data.position.on_bridge)
    {
        // Saliendo del puente
        std::cout << "[BRIDGE] Player leaving bridge (CAR_BRIDGE -> CAR_GROUND)" << std::endl;
        set_car_category(player_data, CAR_GROUND);
        player_data.position.on_bridge = false;
    }

    return player_data.position.on_bridge;
}

void GameLoop::update_bridge_state_for_npc(NPCData &npc_data)
{
    if (!npc_data.body)
    {
        return;
    }

    bool entering_bridge = false;
    bool leaving_bridge = false;

    b2ContactEdge *ce = npc_data.body->GetContactList();
    while (ce)
    {
        b2Contact *c = ce->contact;

        if (c->IsTouching())
        {
            b2Fixture *a = c->GetFixtureA();
            b2Fixture *b = c->GetFixtureB();

            uint16 fcA = a->GetFilterData().categoryBits;
            uint16 fcB = b->GetFilterData().categoryBits;

            // Detectar entrada al puente
            if (fcA == SENSOR_START_BRIDGE || fcB == SENSOR_START_BRIDGE)
            {
                entering_bridge = true;
            }

            // Detectar salida del puente
            if (fcA == SENSOR_END_BRIDGE || fcB == SENSOR_END_BRIDGE)
            {
                leaving_bridge = true;
            }
        }

        ce = ce->next;
    }

    // Actualizar el estado del puente
    if (entering_bridge && !npc_data.on_bridge)
    {
        // Entrando al puente
        std::cout << "[BRIDGE] Player entering bridge (CAR_GROUND -> CAR_BRIDGE)" << std::endl;
        set_npc_category(npc_data, CAR_BRIDGE);
        npc_data.on_bridge = true;
    }
    else if (leaving_bridge && npc_data.on_bridge)
    {
        // Saliendo del puente
        std::cout << "[BRIDGE] Player leaving bridge (CAR_BRIDGE -> CAR_GROUND)" << std::endl;
        set_npc_category(npc_data, CAR_GROUND);
        npc_data.on_bridge = false;
    }
}

void GameLoop::set_car_category(PlayerData &player_data, uint16 newCategory)
{
    b2Body *body = player_data.body;

    for (b2Fixture *f = body->GetFixtureList(); f; f = f->GetNext())
    {
        b2Filter filter = f->GetFilterData();
        filter.categoryBits = newCategory;

        if (newCategory == CAR_GROUND)
        {
            // Auto en el SUELO: ve suelo, otros autos en suelo, NPCs, y sensores
            filter.maskBits =
                COLLISION_FLOOR |     // Colisiones del suelo
                CAR_GROUND |          // Otros jugadores en el suelo
                SENSOR_START_BRIDGE | // Sensor de entrada al puente
                SENSOR_END_BRIDGE;    // Sensor de salida del puente
        }
        else if (newCategory == CAR_BRIDGE)
        {
            // Auto EN EL PUENTE: ve puente, cosas sobre puente, otros autos en puente, y sensores
            filter.maskBits =
                COLLISION_BRIDGE |
                COLLISION_UNDER |
                CAR_BRIDGE |
                SENSOR_START_BRIDGE |
                SENSOR_END_BRIDGE;
        }

        f->SetFilterData(filter);
    }
}

void GameLoop::set_npc_category(NPCData &npc_data, uint16 newCategory)
{
    b2Body *body = npc_data.body;

    for (b2Fixture *f = body->GetFixtureList(); f; f = f->GetNext())
    {
        b2Filter filter = f->GetFilterData();
        filter.categoryBits = newCategory;

        if (newCategory == CAR_GROUND)
        {
            // Auto en el SUELO: ve suelo, otros autos en suelo, NPCs, y sensores
            filter.maskBits =
                COLLISION_FLOOR |     // Colisiones del suelo
                CAR_GROUND |          // Otros jugadores en el suelo
                SENSOR_START_BRIDGE | // Sensor de entrada al puente
                SENSOR_END_BRIDGE;    // Sensor de salida del puente
        }
        else if (newCategory == CAR_BRIDGE)
        {
            // Auto EN EL PUENTE: ve puente, cosas sobre puente, otros autos en puente, y sensores
            filter.maskBits =
                COLLISION_BRIDGE |
                COLLISION_UNDER |
                COLLISION_FLOOR |
                CAR_BRIDGE |
                SENSOR_START_BRIDGE |
                SENSOR_END_BRIDGE;
        }

        f->SetFilterData(filter);
    }
}
