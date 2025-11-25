#include "gameloop.h"
#include "constants.h"
#include "npc_config.h"
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#define MAX_PLAYERS 8
#define INITIAL_X_POS 960
#define INITIAL_Y_POS 540
#define FULL_LOBBY_MSG "can't join lobby, maximum players reached"
#define SCALE 32.0f
#define FPS (1.0f / 60.0f)
#define VELOCITY_ITERS 8
#define COLLISION_ITERS 3


void GameLoop::run()
{
    event_loop.start();
    auto last_tick = std::chrono::steady_clock::now();
    float acum = 0.0f;
    map_layout.create_map_layout("data/cities/liberty_city.json");

    // Extraigo checkpoints del archivo JSON dedicado
    std::vector<b2Vec2> checkpoints;
    map_layout.extract_checkpoints("data/cities/base_liberty_city_checkpoints.json", checkpoints);
    
    if (!checkpoints.empty())
    {
        checkpoint_centers = checkpoints;
        for (size_t i = 0; i < checkpoint_centers.size(); ++i)
        {
            b2BodyDef bd;
            bd.type = b2_staticBody;
            bd.position = checkpoint_centers[i];
            b2Body *body = world.CreateBody(&bd);

            b2CircleShape shape;
            shape.m_p.Set(0.0f, 0.0f);
            shape.m_radius = CHECKPOINT_RADIUS_PX / SCALE;

            b2FixtureDef fd;
            fd.shape = &shape;
            fd.isSensor = true;

            b2Fixture *fix = body->CreateFixture(&fd);
            // Mapeo el fixture al índice del checkpoint
            checkpoint_fixtures[fix] = static_cast<int>(i);
        }
        std::cout << "[GameLoop] Created " << checkpoint_centers.size() << " checkpoint sensors (from base_liberty_city_checkpoints.json)." << std::endl;
    }

    // Cargar configuración de NPCs desde YAML (si existe)
    {
        auto &npc_cfg = NPCConfig::getInstance();
        if (!npc_cfg.loadFromFile("config/npc.yaml")) {
            std::cerr << "[GameLoop][NPC] Using built-in defaults (could not load config/npc.yaml)" << std::endl;
        } else {
            std::cout << "[GameLoop][NPC] Loaded NPC config: moving=" << npc_cfg.getMaxMoving()
                      << " parked=" << npc_cfg.getMaxParked()
                      << " speed_px_s=" << npc_cfg.getSpeedPxS() << std::endl;
        }
    }

    // Extraigo waypoints de NPCs del archivo JSON dedicado
    map_layout.extract_npc_waypoints("data/cities/npc_waypoints.json", street_waypoints);
    
    if (!street_waypoints.empty())
    {
        std::cout << "[GameLoop] Loaded " << street_waypoints.size() << " NPC waypoints (from npc_waypoints.json)." << std::endl;
    }

    // Extraigo autos estacionados del archivo JSON dedicado
    std::vector<MapLayout::ParkedCarData> parked_data;
    map_layout.extract_parked_cars("data/cities/parked_cars.json", parked_data);
    
    // Inicializo NPCs en el mundo
    init_npcs(parked_data);

    while (should_keep_running())
    {
        try
        {
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - last_tick).count(); // segundos
            last_tick = now;
            acum += dt;
            
            // Solo procesar física si el juego está en PLAYING
            if (game_state == GameState::PLAYING && !players.empty())
            {
                update_npcs();
                // Actualiza las propiedades de los bodies de los jugadores según su Position
                update_body_positions();
                // Step del mundo (Hace el cálculo de las físicas y demás
                while (acum >= FPS)
                {
                    world.Step(FPS, VELOCITY_ITERS, COLLISION_ITERS);
                    acum -= FPS;
                }
                std::vector<PlayerPositionUpdate> broadcast;
                // Actulza las posiciones de los jugadores según los bodies y reescala para enviar a clientes
                update_player_positions(broadcast);
                ServerMessage msg;
                msg.opcode = UPDATE_POSITIONS;
                msg.positions = broadcast;

                broadcast_positions(msg);
            }
            else if (game_state == GameState::LOBBY && !players.empty())
            {
                // En lobby: enviar posiciones sin actualizar física
                std::vector<PlayerPositionUpdate> broadcast;
                update_player_positions(broadcast);
                ServerMessage msg;
                msg.opcode = UPDATE_POSITIONS;
                msg.positions = broadcast;
                broadcast_positions(msg);
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

        // Throttle tick rate to ~60 FPS to avoid flooding network/CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    event_loop.stop();
    event_loop.join();
}

// Constructor para poder setear el contact listener del world
GameLoop::GameLoop(std::shared_ptr<Queue<Event>> events)
    : players_map_mutex(), players(), players_messanger(), event_queue(events), event_loop(players_map_mutex, players, event_queue), started(false), game_state(GameState::LOBBY), next_id(INITIAL_ID), map_layout(world), physics_config(CarPhysicsConfig::getInstance())
{
    if (!physics_config.loadFromFile("config/car_physics.yaml")) {
        std::cerr << "[GameLoop] WARNING: Failed to load car physics config, using defaults" << std::endl;
    }

    // seteo el contact listener owner y lo registro con el world
    contact_listener.set_owner(this);
    world.SetContactListener(&contact_listener);
}

void GameLoop::CheckpointContactListener::BeginContact(b2Contact *contact)
{
    if (!owner)
        return;
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();
    owner->handle_begin_contact(a, b);
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

b2Vec2 GameLoop::get_lateral_velocity(b2Body *body) const
{
    b2Vec2 currentRightNormal = body->GetWorldVector(b2Vec2(1, 0));
    return b2Dot(currentRightNormal, body->GetLinearVelocity()) * currentRightNormal;
}

b2Vec2 GameLoop::get_forward_velocity(b2Body *body) const
{
    b2Vec2 currentForwardNormal = body->GetWorldVector(b2Vec2(0, 1));
    return b2Dot(currentForwardNormal, body->GetLinearVelocity()) * currentForwardNormal;
}

void GameLoop::update_friction_for_player(PlayerData &player_data)
{
    b2Body *body = player_data.body;
    if (!body) return;

    const CarPhysics& car_physics = physics_config.getCarPhysics(player_data.car.car_name);

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

void GameLoop::update_drive_for_player(PlayerData &player_data)
{
    b2Body *body = player_data.body;
    if (!body) return;

    const CarPhysics& car_physics = physics_config.getCarPhysics(player_data.car.car_name);

    bool wantUp = (player_data.position.direction_y == up);
    bool wantDown = (player_data.position.direction_y == down);
    bool wantLeft = (player_data.position.direction_x == left);
    bool wantRight = (player_data.position.direction_x == right);

    float maxForwardSpeed_m = car_physics.max_speed / SCALE;                 
    float maxBackwardSpeed_m = -car_physics.max_speed * car_physics.backward_speed_multiplier / SCALE;        
    float maxAccel_m = car_physics.max_acceleration / SCALE;                 


    if (wantUp || wantDown) {
        float desiredSpeed = 0.0f;
        if (wantUp) desiredSpeed = maxForwardSpeed_m;
        else if (wantDown) desiredSpeed = maxBackwardSpeed_m;

        b2Vec2 forwardNormal = body->GetWorldVector(b2Vec2(0, 1));
        float currentSpeed = b2Dot(body->GetLinearVelocity(), forwardNormal);

        float accelCmd = (desiredSpeed - currentSpeed) * car_physics.speed_controller_gain;
        if (accelCmd > maxAccel_m) accelCmd = maxAccel_m;
        if (accelCmd < -maxAccel_m) accelCmd = -maxAccel_m;

        float desiredForce = body->GetMass() * accelCmd;
        body->ApplyForce(desiredForce * forwardNormal, body->GetWorldCenter(), true);
    }

    // aplica un torque modesto cuando se presionan las teclas izquierda/derecha.
    if (wantLeft)
        body->ApplyTorque(-car_physics.torque, true);
    else if (wantRight)
        body->ApplyTorque(car_physics.torque, true);
}

void GameLoop::process_pair(b2Fixture *maybePlayerFix, b2Fixture *maybeCheckpointFix)
{
    if (!maybePlayerFix || !maybeCheckpointFix)
        return;
    b2Body *playerBody = maybePlayerFix->GetBody();
    // Veo si el body pertenece a un jugador
    int playerId = find_player_by_body(playerBody);
    if (playerId < 0)
        return;
    // Veo si el otro fixture es un checkpoint
    auto it = checkpoint_fixtures.find(maybeCheckpointFix);
    if (it == checkpoint_fixtures.end())
        return;
    
    int checkpointIndex = it->second;
    PlayerData &pd = players[playerId];
    // Si el checkpoint que acaba de tocar es el siguiente que debe pasar
    if (pd.next_checkpoint == checkpointIndex)
    {
        int total = static_cast<int>(checkpoint_centers.size());
        int new_next = pd.next_checkpoint + 1;
        if (total > 0 && new_next >= total)
        {
            // Jugador completó todos los checkpoints en la lista
            pd.laps_completed += 1;
            pd.next_checkpoint = 0; // reseteo al primer checkpoint
            std::cout << "[GameLoop] Player " << playerId << " completed all checkpoints! laps=" << pd.laps_completed << std::endl;
        }
        else
        {
            pd.next_checkpoint = new_next;
            std::cout << "[GameLoop] Player " << playerId << " passed checkpoint " << checkpointIndex << " next=" << pd.next_checkpoint << std::endl;
        }
    }
}

void GameLoop::handle_begin_contact(b2Fixture *a, b2Fixture *b)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    process_pair(a, b);
    process_pair(b, a);
}

void GameLoop::add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    std::cout << "[GameLoop] add_player: id=" << id
              << " players.size()=" << players.size()
              << " outbox.valid=" << std::boolalpha << static_cast<bool>(player_outbox)
              << std::endl;
    
    // Verificar que no excedamos el máximo de jugadores
    if (int(players.size()) >= MAX_PLAYERS) {
        std::cout << FULL_LOBBY_MSG << std::endl;
        return;
    }
    
    // Agregar jugador al orden
    player_order.push_back(id);
    
    // Calcular índice de spawn basado en la posición en el orden
    int spawn_idx = static_cast<int>(player_order.size()) - 1;
    const SpawnPoint& spawn = spawn_points[spawn_idx];
    
    std::cout << "[GameLoop] add_player: assigning spawn point " << spawn_idx 
              << " at (" << spawn.x << "," << spawn.y << ")" << std::endl;
    
    Position pos = Position{spawn.x, spawn.y, not_horizontal, not_vertical, spawn.angle};
    players[id] = PlayerData{
        create_player_body(spawn.x, spawn.y, pos, "defaults"),
        MOVE_UP_RELEASED_STR,
        CarInfo{"defaults", DEFAULT_CAR_SPEED_PX_S, DEFAULT_CAR_ACCEL_PX_S2, DEFAULT_CAR_HP},
        pos
    };
    players_messanger[id] = player_outbox;
    
    std::cout << "[GameLoop] add_player: done. players.size()=" << players.size()
              << " messengers.size()=" << players_messanger.size() << std::endl;
}

void GameLoop::remove_player(int client_id)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    auto it = players.find(client_id);
    if (it == players.end())
    {
        throw std::runtime_error("player not found");
    }
    
    // Destruir el body del jugador
    PlayerData &pd = it->second;
    b2Body *body = pd.body;
    if (body)
    {
        b2World *w = body->GetWorld();
        if (w)
        {
            try {
                w->DestroyBody(body);
            } catch (...) {
                std::cerr << "[GameLoop] Warning: DestroyBody threw for client " << client_id << std::endl;
            }
        }
    }
    
    // Borrar del mapa de mensajería y players
    players_messanger.erase(client_id);
    players.erase(it);
    
    // Remover del orden de jugadores
    auto order_it = std::find(player_order.begin(), player_order.end(), client_id);
    if (order_it != player_order.end()) {
        player_order.erase(order_it);
    }
    
    std::cout << "[GameLoop] remove_player: client " << client_id << " removed" << std::endl;
    
    // Si estamos en LOBBY, reordenar los jugadores restantes
    if (game_state == GameState::LOBBY && !player_order.empty()) {
        std::cout << "[GameLoop] remove_player: reordering remaining " << player_order.size() << " players in lobby" << std::endl;
        
        // Reposicionar cada jugador restante a su nuevo spawn point
        for (size_t i = 0; i < player_order.size(); ++i) {
            int player_id = player_order[i];
            auto player_it = players.find(player_id);
            if (player_it == players.end()) continue;
            
            PlayerData& player_data = player_it->second;
            const SpawnPoint& spawn = spawn_points[i];
            
            std::cout << "[GameLoop] remove_player: moving player " << player_id 
                      << " to spawn " << i << " at (" << spawn.x << "," << spawn.y << ")" << std::endl;
            
            // Destruir el body anterior
            if (player_data.body) {
                b2World* world = player_data.body->GetWorld();
                if (world) {
                    world->DestroyBody(player_data.body);
                }
            }
            
            // Crear nuevo body en la nueva posición
            Position new_pos{spawn.x, spawn.y, not_horizontal, not_vertical, spawn.angle};
            player_data.body = create_player_body(spawn.x, spawn.y, new_pos, player_data.car.car_name);
            player_data.position = new_pos;
        }
    }
}

void GameLoop::start_game()
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    if (game_state == GameState::LOBBY) {
        game_state = GameState::PLAYING;
        std::cout << "[GameLoop] Game started! Transitioning from LOBBY to PLAYING" << std::endl;
        
        // Reset velocities and positions to ensure clean start
        for (auto &[id, player_data] : players) {
            if (player_data.body) {
                // Reset velocidades lineales y angulares
                player_data.body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
                player_data.body->SetAngularVelocity(0.0f);
                
                // Sincronizar la posición del body con la posición almacenada
                b2Vec2 current_pos = player_data.body->GetPosition();
                float current_x_px = current_pos.x * SCALE;
                float current_y_px = current_pos.y * SCALE;
                
                std::cout << "[GameLoop] start_game: Player " << id 
                          << " at body_pos=(" << current_x_px << "," << current_y_px << ")"
                          << " stored_pos=(" << player_data.position.new_X << "," << player_data.position.new_Y << ")"
                          << std::endl;
            }
        }
    }
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

b2Body *GameLoop::create_player_body(float x_px, float y_px, Position &pos, const std::string& car_name)
{
    const CarPhysics& car_physics = physics_config.getCarPhysics(car_name);

    // Crecion del body
    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position.Set(x_px / SCALE, y_px / SCALE);

    // Rotacion del body: use explicit angle provided in Position (radians)
    bd.angle = pos.angle;

    // Lo creamos en el world
    b2Body *b = world.CreateBody(&bd);

    float halfWidth = car_physics.width / (2.0f * SCALE);
    float halfHeight = car_physics.height / (2.0f * SCALE);

    // Offset del centro de colisión (Y+ = hacia adelante del auto)
    b2Vec2 center_offset(0.0f, car_physics.center_offset_y / SCALE);
    
    b2PolygonShape shape;
    shape.SetAsBox(halfWidth, halfHeight, center_offset, 0.0f);

    b2FixtureDef fd;
    fd.shape = &shape;
    fd.density = car_physics.density;
    fd.friction = car_physics.friction;
    fd.restitution = car_physics.restitution;

    b->CreateFixture(&fd);

    b->SetBullet(true);  // mejora CCD para objetos rápidos
    b->SetLinearDamping(car_physics.linear_damping);
    b->SetAngularDamping(car_physics.angular_damping);

    return b;
}

void GameLoop::update_player_positions(std::vector<PlayerPositionUpdate> &broadcast)
{
    std::lock_guard<std::mutex> lk(players_map_mutex);
    for (auto &[id, player_data] : players)
    {
        b2Body *body = player_data.body;
        b2Vec2 p = body->GetPosition();
        player_data.position.new_X = p.x * SCALE; // reconvertir a píxeles
        player_data.position.new_Y = p.y * SCALE;
        double ang = body->GetAngle();
        while (ang < 0.0) ang += 2.0 * M_PI;
        while (ang >= 2.0 * M_PI) ang -= 2.0 * M_PI;
        player_data.position.angle = static_cast<float>(ang);

        PlayerPositionUpdate update;
        update.player_id = id;
        update.new_pos = player_data.position;
    update.car_type = player_data.car.car_name;

        // Envio los 3 proximos checkpoints
        const int LOOKAHEAD = 3;
        if (!checkpoint_centers.empty())
        {
            int total = static_cast<int>(checkpoint_centers.size());
            for (int k = 0; k < LOOKAHEAD; ++k)
            {
                int idx = (player_data.next_checkpoint + k) % total;
                b2Vec2 c = checkpoint_centers[idx];
                Position cp_pos{c.x * SCALE, c.y * SCALE, not_horizontal, not_vertical, 0.0f};
                update.next_checkpoints.push_back(cp_pos);
            }
        }

        broadcast.push_back(update);

        // Printeo la posición del jugador y su próximo checkpoint
        // int next_idx = player_data.next_checkpoint;
        // if (next_idx >= 0 && next_idx < static_cast<int>(checkpoint_centers.size()))
        // {
        //     b2Vec2 c = checkpoint_centers[next_idx];
        //     float cx_px = c.x * SCALE;
        //     float cy_px = c.y * SCALE;
        //     std::cout << "[GameLoop] Player " << id << " pos=(" << player_data.position.new_X << "," << player_data.position.new_Y << ") "
        //               << "next_checkpoint=(" << cx_px << "," << cy_px << ") idx=" << next_idx << std::endl;
        // }
    }

    // Apendear también las posiciones de los NPCs
    for (auto &npc : npcs) {
        if (!npc.body) continue;
        b2Vec2 p = npc.body->GetPosition();
        Position pos{};
        pos.new_X = p.x * SCALE;
        pos.new_Y = p.y * SCALE;
        
        b2Vec2 vel = npc.body->GetLinearVelocity();
        const float DIR_THRESH = 0.05f; // umbral para considerar movimiento significativo
        MovementDirectionX dx = not_horizontal;
        MovementDirectionY dy = not_vertical;
        if (vel.x > DIR_THRESH) dx = right; else if (vel.x < -DIR_THRESH) dx = left;
        if (vel.y > DIR_THRESH) dy = down; else if (vel.y < -DIR_THRESH) dy = up;
        pos.direction_x = dx;
        pos.direction_y = dy;
        // usar ángulo real del body (normalizar a [0,2PI)) para que el cliente pueda orientar correctamente
        double ang = npc.body->GetAngle();
        while (ang < 0.0) ang += 2.0 * M_PI;
        while (ang >= 2.0 * M_PI) ang -= 2.0 * M_PI;
        pos.angle = static_cast<float>(ang);

        PlayerPositionUpdate update;
        update.player_id = npc.npc_id; // id negativo para NPC
        update.new_pos = pos;
        update.car_type = "npc";
        broadcast.push_back(update);
    }
}

void GameLoop::update_body_positions()
{
    // Velocidad del body (en metros/seg)
    std::lock_guard<std::mutex> lk(players_map_mutex);
    for (auto &[id, player_data] : players)
    {
        // Aplicar fricción/adhesión primero
        update_friction_for_player(player_data);

        // Aplicar fuerza de conducción / torque basado en la entrada
        update_drive_for_player(player_data);
    }
}

// ---------------- NPC Implementation ----------------

b2Body* GameLoop::create_npc_body(float x_px, float y_px, bool is_static, float angle_rad) {
    b2BodyDef bd;
    bd.type = is_static ? b2_staticBody : b2_kinematicBody; // static para estacionados, kinematic para móviles
    bd.position.Set(x_px / SCALE, y_px / SCALE);
    bd.angle = angle_rad;  // Establecer la orientación inicial
    b2Body* b = world.CreateBody(&bd);

    // Igual q el jugador
    float halfWidth = 22.0f / (2.0f * SCALE);
    float halfHeight = 28.0f / (2.0f * SCALE);
    b2PolygonShape shape; shape.SetAsBox(halfWidth, halfHeight);
    b2FixtureDef fd; fd.shape = &shape; fd.isSensor = false; fd.density = 1.0f; fd.friction = 0.3f; fd.restitution = 0.0f;
    b->CreateFixture(&fd);
    return b;
}

void GameLoop::init_npcs(const std::vector<MapLayout::ParkedCarData> &parked_data) {
    std::lock_guard<std::mutex> lk(players_map_mutex); 
    
    int next_negative_id = -1; // -1, -2, -3 ...
    
    // 1. Crear NPCs estacionados desde parked_cars.json (limitado a MAX_PARKED_NPCS)
    int parked_count = std::min(static_cast<int>(parked_data.size()), NPCConfig::getInstance().getMaxParked());
    
    // Mezclar aleatoriamente las posiciones disponibles
    std::random_device rd_parked;
    std::mt19937 gen_parked(rd_parked());
    
    // Si hay más posiciones que el límite, seleccionar aleatoriamente
    std::vector<size_t> parked_indices;
    for (size_t i = 0; i < parked_data.size(); ++i) {
        parked_indices.push_back(i);
    }
    std::shuffle(parked_indices.begin(), parked_indices.end(), gen_parked);
    
    // Inicializar NPCs estacionados
    for (int i = 0; i < parked_count; ++i) {
        const auto& parked = parked_data[parked_indices[i]];
        
        // Determinar ángulo según orientación
        float angle_rad = parked.horizontal ? b2_pi / 2.0f : 0.0f;  // horizontal=true son 90°, horizontal=false es 0°
        
        b2Body* body = create_npc_body(parked.position.x * SCALE, parked.position.y * SCALE, true, angle_rad);
        
        NPCData npc;
        npc.body = body;
        npc.npc_id = next_negative_id--;
        npc.current_waypoint = -1;  // No tiene waypoint asociado
        npc.target_waypoint = -1;
        npc.speed_mps = 0.0f;
        npc.is_parked = true;
        npc.is_horizontal = parked.horizontal;
        
        npcs.push_back(npc);
    }
    
    std::cout << "[GameLoop][NPC] Spawned " << parked_count << " parked NPCs (out of " << parked_data.size() << " available positions)." << std::endl;
    
    // 2. Crear NPCs móviles desde waypoints
    if (street_waypoints.size() < 2) {
        std::cout << "[GameLoop][NPC] Not enough waypoints to spawn moving NPCs." << std::endl;
        return;
    }
    
    // Spawnnear hasta MAX_MOVING_NPCS NPCs móviles
    int moving_npcs_count = std::min(NPCConfig::getInstance().getMaxMoving(), static_cast<int>(street_waypoints.size()));
    
    std::random_device rd; 
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> wp_dist(0, static_cast<int>(street_waypoints.size()) - 1);

    for (int i = 0; i < moving_npcs_count; ++i) {
        int start_wp_idx = wp_dist(gen);
        const MapLayout::WaypointData& start_wp = street_waypoints[start_wp_idx];
        
        b2Vec2 spawn_pos = start_wp.position;
    float speed_mps = NPCConfig::getInstance().getSpeedPxS() / SCALE;
        
        // Crear cuerpo kinematic (móvil)
        b2Body* body = create_npc_body(spawn_pos.x * SCALE, spawn_pos.y * SCALE, false);
        
        // Elegir target inicial aleatorio de los waypoints conectados
        int target_wp_idx = start_wp_idx;
        if (!start_wp.connections.empty()) {
            std::uniform_int_distribution<size_t> conn_dist(0, start_wp.connections.size() - 1);
            target_wp_idx = start_wp.connections[conn_dist(gen)];
        }
        
        NPCData npc;
        npc.body = body;
        npc.npc_id = next_negative_id--;
        npc.current_waypoint = start_wp_idx;
        npc.target_waypoint = target_wp_idx;
        npc.speed_mps = speed_mps;
        npc.is_parked = false;
        npc.is_horizontal = false;  // No aplica para móviles
        
        npcs.push_back(npc);
    }
    
    std::cout << "[GameLoop][NPC] Spawned " << moving_npcs_count << " moving NPCs." << std::endl;
    std::cout << "[GameLoop][NPC] Total NPCs: " << npcs.size() << std::endl;
}

void GameLoop::update_npcs() {
    std::lock_guard<std::mutex> lk(players_map_mutex); 
    if (street_waypoints.empty()) return;
    
    const float arrival_threshold_m = 0.5f; // 0.5 metros para considerar "llegado" al waypoint
    std::random_device rd; 
    std::mt19937 gen(rd());
    
    for (auto &npc : npcs) {
        b2Body* body = npc.body;
        if (!body || npc.is_parked) continue; // NPCs estacionados no se mueven
        
        // Validar índices
        if (npc.target_waypoint < 0 || npc.target_waypoint >= static_cast<int>(street_waypoints.size())) {
            continue;
        }
        
        b2Vec2 target_pos = street_waypoints[npc.target_waypoint].position;
        b2Vec2 pos = body->GetPosition();
        b2Vec2 to_target = target_pos - pos;
        float dist = to_target.Length();
        
        // Si llegó al waypoint objetivo, elegir siguiente destino aleatorio
        if (dist < arrival_threshold_m) {
            npc.current_waypoint = npc.target_waypoint;
            const MapLayout::WaypointData& current_wp = street_waypoints[npc.current_waypoint];
            
            // Elegir aleatoriamente uno de los waypoints conectados
            if (!current_wp.connections.empty()) {
                std::uniform_int_distribution<size_t> conn_dist(0, current_wp.connections.size() - 1);
                npc.target_waypoint = current_wp.connections[conn_dist(gen)];
                
                // Recalcular para el nuevo target
                target_pos = street_waypoints[npc.target_waypoint].position;
                to_target = target_pos - pos;
                dist = to_target.Length();
            }
        }
        
        // Moverse hacia el target
        if (dist > 0.0001f) {
            b2Vec2 dir = (1.0f / dist) * to_target; // normalizar
            b2Vec2 vel = npc.speed_mps * dir;
            body->SetLinearVelocity(vel);
            
            // Orientar el auto en la dirección del movimiento
            float movement_angle = std::atan2(vel.y, vel.x); 
            float body_angle = movement_angle - b2_pi/2.0f; // ajustar por sprite orientado hacia arriba
            body->SetTransform(pos, body_angle);
        } else {
            body->SetLinearVelocity(b2Vec2(0,0));
        }
    }
}

size_t GameLoop::get_player_count() const {
    std::lock_guard<std::mutex> lk(const_cast<std::mutex&>(players_map_mutex));
    return players.size();
}
