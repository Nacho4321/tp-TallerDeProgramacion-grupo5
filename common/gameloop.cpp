#include "gameloop.h"
#include "constants.h"
#include <thread>
#include <chrono>
#include <random>
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

    // Initialize NPCs after checkpoints are available
    init_npcs();

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
                // Update NPC movement even if there are players (NPCs depend only on waypoints)
                update_npcs();
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
    : players_map_mutex(), players(), players_messanger(), event_queue(events), event_loop(players_map_mutex, players, event_queue), started(false), next_id(INITIAL_ID), map_layout(world)
{
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

    // impulso lateral para reducir el deslizamiento lateral (limitado para permitir derrapes)
    const float maxLateralImpulse = 2.5f;
    b2Vec2 impulse = body->GetMass() * -get_lateral_velocity(body);
    float ilen = impulse.Length();
    if (ilen > maxLateralImpulse)
        impulse *= maxLateralImpulse / ilen;
    body->ApplyLinearImpulse(impulse, body->GetWorldCenter(), true);

    // matar un poco la velocidad angular para evitar giros descontrolados
    body->ApplyAngularImpulse(0.1f * body->GetInertia() * -body->GetAngularVelocity(), true);

    // forward drag
    b2Vec2 forwardDir = body->GetWorldVector(b2Vec2(0, 1));
    float currentForwardSpeed = b2Dot(body->GetLinearVelocity(), forwardDir);
    b2Vec2 dragForce = -2.0f * currentForwardSpeed * forwardDir;
    body->ApplyForce(dragForce, body->GetWorldCenter(), true);
}

void GameLoop::update_drive_for_player(PlayerData &player_data)
{
    b2Body *body = player_data.body;
    if (!body) return;

    bool wantUp = (player_data.position.direction_y == up);
    bool wantDown = (player_data.position.direction_y == down);
    bool wantLeft = (player_data.position.direction_x == left);
    bool wantRight = (player_data.position.direction_x == right);

    float maxForwardSpeed_m = player_data.car.speed / SCALE;                 // m/s
    float maxBackwardSpeed_m = -player_data.car.speed * 0.5f / SCALE;        // m/s
    float maxAccel_m = player_data.car.acceleration / SCALE;                 // m/s^2


    float desiredSpeed = 0.0f;
    if (wantUp) desiredSpeed = maxForwardSpeed_m;
    else if (wantDown) desiredSpeed = maxBackwardSpeed_m;
    else return; 


    b2Vec2 forwardNormal = body->GetWorldVector(b2Vec2(0, 1));
    float currentSpeed = b2Dot(body->GetLinearVelocity(), forwardNormal);

    const float kp = 4.0f;
    float accelCmd = (desiredSpeed - currentSpeed) * kp;
    if (accelCmd > maxAccel_m) accelCmd = maxAccel_m;
    if (accelCmd < -maxAccel_m) accelCmd = -maxAccel_m;

    // Force = mass * accel
    float desiredForce = body->GetMass() * accelCmd;
    body->ApplyForce(desiredForce * forwardNormal, body->GetWorldCenter(), true);

    // aplica un torque modesto cuando se presionan las teclas izquierda/derecha.
    const float turnTorque = 2.0f; 
    if (wantLeft)
        body->ApplyTorque(-turnTorque, true);
    else if (wantRight)
        body->ApplyTorque(turnTorque, true);
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
    if (int(players.size()) == 0)
    {
        Position pos = Position{INITIAL_X_POS, INITIAL_Y_POS, not_horizontal, not_vertical, 0.0f};
        players[id] = PlayerData{create_player_body(INITIAL_X_POS, INITIAL_Y_POS, pos),
                                 MOVE_UP_RELEASED_STR,
                                 CarInfo{"lambo", DEFAULT_CAR_SPEED_PX_S, DEFAULT_CAR_ACCEL_PX_S2, DEFAULT_CAR_HP}, pos};
        players_messanger[id] = player_outbox;
    }
    else if (int(players.size()) < MAX_PLAYERS)
    {
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
    players[id] = PlayerData{create_player_body(dir_x, dir_y, pos),
                MOVE_UP_RELEASED_STR, CarInfo{"lambo", DEFAULT_CAR_SPEED_PX_S, DEFAULT_CAR_ACCEL_PX_S2, DEFAULT_CAR_HP}, pos};
        std::cout << "[GameLoop] add_player: player data inserted" << std::endl;
        players_messanger[id] = player_outbox;
        std::cout << "[GameLoop] add_player: messenger inserted" << std::endl;
    }
    else
    {
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

b2Body *GameLoop::create_player_body(float x_px, float y_px, Position &pos)
{
    // Crecion del body
    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position.Set(x_px / SCALE, y_px / SCALE);

    // Rotacion del body: use explicit angle provided in Position (radians)
    bd.angle = pos.angle;

    // Lo creamos en el world
    b2Body *b = world.CreateBody(&bd);

    // Tamaño del sprite en metros (En teoría segun vi en renderer es 28x22 px)
    float halfWidth = 22.0f / (2.0f * SCALE);
    float halfHeight = 28.0f / (2.0f * SCALE);

    b2PolygonShape shape;
    shape.SetAsBox(halfWidth, halfHeight);

    // Fixture son las propiedades físicas del body
    b2FixtureDef fd;
    fd.shape = &shape;
    fd.density = 1.0f;
    fd.friction = 0.3f;
    fd.restitution = 0.1f;

    b->CreateFixture(&fd);

    b->SetBullet(true);        // mejora CCD para objetos rápidos
    b->SetLinearDamping(1.0f); // frena un poco naturalmente
    b->SetAngularDamping(1.0f);

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

        int next_idx = player_data.next_checkpoint;
        if (next_idx >= 0 && next_idx < static_cast<int>(checkpoint_centers.size()))
        {
            b2Vec2 c = checkpoint_centers[next_idx];
            float cx_px = c.x * SCALE;
            float cy_px = c.y * SCALE;
            std::cout << "[GameLoop] Player " << id << " pos=(" << player_data.position.new_X << "," << player_data.position.new_Y << ") "
                      << "angle_rad=" << player_data.position.angle << " next_checkpoint=(" << cx_px << "," << cy_px << ") idx=" << next_idx << std::endl;
        }
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

b2Body* GameLoop::create_npc_body(float x_px, float y_px) {
    b2BodyDef bd;
    bd.type = b2_kinematicBody; // kinematic: controlamos la velocidad directamente
    bd.position.Set(x_px / SCALE, y_px / SCALE);
    b2Body* b = world.CreateBody(&bd);

    // Igual q el jugador
    float halfWidth = 22.0f / (2.0f * SCALE);
    float halfHeight = 28.0f / (2.0f * SCALE);
    b2PolygonShape shape; shape.SetAsBox(halfWidth, halfHeight);
    b2FixtureDef fd; fd.shape = &shape; fd.isSensor = false; fd.density = 1.0f; fd.friction = 0.3f; fd.restitution = 0.0f;
    b->CreateFixture(&fd);
    return b;
}

void GameLoop::init_npcs() {
    std::lock_guard<std::mutex> lk(players_map_mutex); 
    if (checkpoint_centers.size() < 2) {
        std::cout << "[GameLoop][NPC] Not enough checkpoints to spawn moving NPCs." << std::endl;
        return;
    }
    int desired_npcs = std::min(MAX_NPCS, static_cast<int>(checkpoint_centers.size()));
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<int> wp_dist(0, static_cast<int>(checkpoint_centers.size()) - 1);
    std::uniform_real_distribution<float> dir_dist(0.0f, 1.0f);

    int next_negative_id = -1; // -1, -2, -3 ...
    for (int i = 0; i < desired_npcs; ++i) {
        int start_wp = wp_dist(gen);
        int step = (dir_dist(gen) < NPC_REVERSE_RATIO) ? -1 : 1;
        b2Vec2 wp = checkpoint_centers[start_wp];
        float speed_mps = NPC_SPEED_PX_S / SCALE;
        b2Body* body = create_npc_body(wp.x * SCALE, wp.y * SCALE);
        NPCData npc{body, next_negative_id--, start_wp, step, speed_mps};
        npcs.push_back(npc);
    }
    std::cout << "[GameLoop][NPC] Spawned " << npcs.size() << " NPCs." << std::endl;
}

void GameLoop::update_npcs() {
    std::lock_guard<std::mutex> lk(players_map_mutex); 
    if (checkpoint_centers.empty()) return;
    const float arrival_threshold_m = 0.5f; // 0.5 metros para considerar "llegado" al waypoint
    int total = static_cast<int>(checkpoint_centers.size());
    for (auto &npc : npcs) {
        b2Body* body = npc.body;
        if (!body) continue;
        b2Vec2 target = checkpoint_centers[npc.waypoint_index];
        b2Vec2 pos = body->GetPosition();
        b2Vec2 to_target = target - pos;
        float dist = to_target.Length();
        if (dist < arrival_threshold_m) {
            npc.waypoint_index = (npc.waypoint_index + npc.direction_step + total) % total;
            target = checkpoint_centers[npc.waypoint_index];
            to_target = target - pos;
            dist = to_target.Length();
        }
        if (dist > 0.0001f) {
            b2Vec2 dir = (1.0f / dist) * to_target; // normalizo
            b2Vec2 vel = npc.speed_mps * dir;
            body->SetLinearVelocity(vel);
            float movement_angle = std::atan2(vel.y, vel.x); 
            float body_angle = movement_angle - b2_pi/2.0f; 
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
