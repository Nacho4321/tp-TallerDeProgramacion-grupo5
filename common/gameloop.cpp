#include "gameloop.h"
#include "constants.h"
#include <thread>
#include <chrono>
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

    while (should_keep_running())
    {
        try
        {
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - last_tick).count(); // segundos
            last_tick = now;
            acum += dt;
            if (!players.empty())
            {
                // Actualiza las propiedades de los bodies de los jugadores según su Position
                update_body_positions();
                // Step del mundo (Hace el cálculo de las físicas y demás, no es nuestra incumbencia como lo hace)
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
    : players_map_mutex(), players(), players_messanger(), event_queue(events), event_loop(players_map_mutex, players, event_queue), started(false), next_id(INITIAL_ID), map_layout(world), physics_config(CarPhysicsConfig::getInstance())
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

    // aplica un torque cuando se presionan las teclas izquierda/derecha
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
    std::vector<PlayerPositionUpdate> broadcast;
    if (int(players.size()) == 0) {
        Position pos = Position{INITIAL_X_POS, INITIAL_Y_POS, not_horizontal, not_vertical, 0.0f};
        players[id] = PlayerData{create_player_body(INITIAL_X_POS, INITIAL_Y_POS, pos, "defaults"),
                                 MOVE_UP_RELEASED_STR,
                                 CarInfo{"defaults", DEFAULT_CAR_SPEED_PX_S, DEFAULT_CAR_ACCEL_PX_S2, DEFAULT_CAR_HP}, pos};
        players_messanger[id] = player_outbox;
    }
    else if (int(players.size()) < MAX_PLAYERS) {
        std::cout << "[GameLoop] add_player: computing spawn near an existing player" << std::endl;
        auto anchor_it = players.begin();
        float dir_x = INITIAL_X_POS;
        float dir_y = INITIAL_Y_POS;
        if (anchor_it != players.end())
        {
            std::cout << "[GameLoop] add_player: anchor id=" << anchor_it->first << std::endl;
            dir_x = anchor_it->second.position.new_X + 30.0f * static_cast<float>(players.size());
            dir_y = anchor_it->second.position.new_Y;
        }
        std::cout << "[GameLoop] add_player: spawn at (" << dir_x << "," << dir_y << ")" << std::endl;
        Position pos = Position{dir_x, dir_y, not_horizontal, not_vertical, 0.0f};
        players[id] = PlayerData{create_player_body(dir_x, dir_y, pos, "defaults"),
                    MOVE_UP_RELEASED_STR, CarInfo{"defaults", DEFAULT_CAR_SPEED_PX_S, DEFAULT_CAR_ACCEL_PX_S2, DEFAULT_CAR_HP}, pos};
        std::cout << "[GameLoop] add_player: player data inserted" << std::endl;
        players_messanger[id] = player_outbox;
        std::cout << "[GameLoop] add_player: messenger inserted" << std::endl;
    }
    else {
        std::cout << FULL_LOBBY_MSG << std::endl;
    }
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
    PlayerData &pd = it->second;
    b2Body *body = pd.body;
    if (body)
    {
        b2World *w = body->GetWorld();
        if (w)
        {
            // Destruir el body del mundo de Box2D
            try {
                w->DestroyBody(body);
            } catch (...) {
                std::cerr << "[GameLoop] Warning: DestroyBody threw for client " << client_id << std::endl;
            }
        }
    }
    // Borrar del mapa de mensajería si existiera
    players_messanger.erase(client_id);
    // Borrar del mapa de players
    players.erase(it);
    std::cout << "[GameLoop] remove_player: client " << client_id << " removed" << std::endl;
}

void GameLoop::start_game()
{
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

    b2PolygonShape shape;
    shape.SetAsBox(halfWidth, halfHeight);

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

size_t GameLoop::get_player_count() const {
    std::lock_guard<std::mutex> lk(const_cast<std::mutex&>(players_map_mutex));
    return players.size();
}
