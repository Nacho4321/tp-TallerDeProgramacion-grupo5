#include "gameloop.h"
#include <thread>
#include <chrono>
#define MAX_PLAYERS 8
#define INITIAL_X_POS 960
#define INITIAL_Y_POS 540
// Velocidad base en pixeles por segundo (antes era 0.0008 por tick, casi imperceptible)
#define INITIAL_SPEED 200.0f
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
        Position pos = Position{INITIAL_X_POS, INITIAL_Y_POS, not_horizontal, not_vertical};
        players[id] = PlayerData{create_player_body(INITIAL_X_POS, INITIAL_Y_POS, pos),
                                 MOVE_UP_RELEASED_STR,
                                 CarInfo{"lambo", INITIAL_SPEED, INITIAL_SPEED, INITIAL_SPEED}, pos};
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
        Position pos = Position{dir_x, dir_y, not_horizontal, not_vertical};
        players[id] = PlayerData{create_player_body(dir_x, dir_y, pos),
                                 MOVE_UP_RELEASED_STR, CarInfo{"lambo", INITIAL_SPEED, INITIAL_SPEED, INITIAL_SPEED}, pos};
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

    // Rotacion del body
    float dy = float(pos.direction_y);
    float dx = float(pos.direction_x);
    float angleRad = std::atan2(dy, dx);
    bd.angle = angleRad;

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

        PlayerPositionUpdate update{id, player_data.position};
        broadcast.push_back(update);
    }
}

void GameLoop::update_body_positions()
{
    // Velocidad del body (en metros/seg)
    std::lock_guard<std::mutex> lk(players_map_mutex);
    for (auto &[id, player_data] : players)
    {
        Position &pos = player_data.position;
        float dirX = float(pos.direction_x);
        float dirY = float(pos.direction_y);

        if (dirX != 0 || dirY != 0)
        {
            float angle = std::atan2(dirY, dirX);
            player_data.body->SetTransform(
                player_data.body->GetPosition(),
                angle);
        }

        // convertimos velocidad (px/s) a m/s para que funcione en box2d como queremos
        float vx = dirX * (player_data.car.speed / SCALE);
        float vy = dirY * (player_data.car.speed / SCALE);

        if (player_data.body)
        {
            player_data.body->SetLinearVelocity(b2Vec2(vx, vy));
        }
    }
}
