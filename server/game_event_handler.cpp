#include "game_event_handler.h"
#include "../common/constants.h"
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_fixture.h>

void GameEventHandler::init_handlers()
{
    listeners[MOVE_UP_PRESSED_STR] = [this](Event &e)
    { move_up(e); };
    listeners[MOVE_UP_RELEASED_STR] = [this](Event &e)
    { move_up_released(e); };
    listeners[MOVE_DOWN_PRESSED_STR] = [this](Event &e)
    { move_down(e); };
    listeners[MOVE_DOWN_RELEASED_STR] = [this](Event &e)
    { move_down_released(e); };
    listeners[MOVE_LEFT_PRESSED_STR] = [this](Event &e)
    { move_left(e); };
    listeners[MOVE_LEFT_RELEASED_STR] = [this](Event &e)
    { move_left_released(e); };
    listeners[MOVE_RIGHT_PRESSED_STR] = [this](Event &e)
    { move_right(e); };
    listeners[MOVE_RIGHT_RELEASED_STR] = [this](Event &e)
    { move_right_released(e); };
    listeners[std::string(CHANGE_CAR_STR) + " " + GREEN_CAR] = [this](Event &e)
    { select_car(e, GREEN_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + RED_SQUARED_CAR] = [this](Event &e)
    { select_car(e, RED_SQUARED_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + RED_SPORTS_CAR] = [this](Event &e)
    { select_car(e, RED_SPORTS_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + LIGHT_BLUE_CAR] = [this](Event &e)
    { select_car(e, LIGHT_BLUE_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + RED_JEEP_CAR] = [this](Event &e)
    { select_car(e, RED_JEEP_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + PURPLE_TRUCK] = [this](Event &e)
    { select_car(e, PURPLE_TRUCK); };
    listeners[std::string(CHANGE_CAR_STR) + " " + LIMOUSINE_CAR] = [this](Event &e)
    { select_car(e, LIMOUSINE_CAR); };
    listeners[std::string(UPGRADE_CAR_STR) + " " + std::to_string(int(CarUpgrade::ACCELERATION_BOOST))] = [this](Event &e)
    { upgrade_max_acceleration(e); };
    listeners[std::string(UPGRADE_CAR_STR) + " " + std::to_string(int(CarUpgrade::SPEED_BOOST))] = [this](Event &e)
    { upgrade_max_speed(e); };
    listeners[std::string(UPGRADE_CAR_STR) + " " + std::to_string(int(CarUpgrade::HANDLING_IMPROVEMENT))] = [this](Event &e)
    { upgrade_handling(e); };
    listeners[std::string(UPGRADE_CAR_STR) + " " + std::to_string(int(CarUpgrade::DURABILITY_ENHANCEMENT))] = [this](Event &e)
    { upgrade_durability(e); };
    listeners[CHEAT_GOD_MODE_STR] = [this](Event &e)
    { cheat_god_mode(e); };
    listeners[CHEAT_DIE_STR] = [this](Event &e)
    { cheat_die(e); };
    listeners[CHEAT_SKIP_LAP_STR] = [this](Event &e)
    { cheat_skip_round(e); };
    listeners[CHEAT_FULL_UPGRADE_STR] = [this](Event &e)
    { cheat_full_upgrade(e); };
}

void GameEventHandler::move_up(Event &event)
{
    if (current_state != GameState::PLAYING)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_y = up;
    players[event.client_id].state = event.action;
}

void GameEventHandler::move_up_released(Event &event)
{
    if (current_state != GameState::PLAYING)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    if (players[event.client_id].state != MOVE_DOWN_PRESSED_STR)
    {
        players[event.client_id].position.direction_y = not_vertical;
        players[event.client_id].state = event.action;
    }
}

void GameEventHandler::move_down(Event &event)
{
    if (current_state != GameState::PLAYING)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_y = down;
    players[event.client_id].state = event.action;
}

void GameEventHandler::move_down_released(Event &event)
{
    if (current_state != GameState::PLAYING)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    if (players[event.client_id].state != MOVE_UP_PRESSED_STR)
    {
        players[event.client_id].position.direction_y = not_vertical;
        players[event.client_id].state = event.action;
    }
}

void GameEventHandler::move_left(Event &event)
{
    if (current_state != GameState::PLAYING)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_x = left;
    players[event.client_id].state = event.action;
}

void GameEventHandler::move_left_released(Event &event)
{
    if (current_state != GameState::PLAYING)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    if (players[event.client_id].state != MOVE_RIGHT_PRESSED_STR)
    {
        players[event.client_id].position.direction_x = not_horizontal;
        players[event.client_id].state = event.action;
    }
}

void GameEventHandler::move_right(Event &event)
{
    if (current_state != GameState::PLAYING)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_x = right;
    players[event.client_id].state = event.action;
}

void GameEventHandler::move_right_released(Event &event)
{
    if (current_state != GameState::PLAYING)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    if (players[event.client_id].state != MOVE_LEFT_PRESSED_STR)
    {
        players[event.client_id].position.direction_x = not_horizontal;
        players[event.client_id].state = event.action;
    }
}
void GameEventHandler::select_car(Event &event, const std::string &car_type)
{
    if (current_state != GameState::LOBBY)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
    {
        return;
    }
    const CarPhysics &phys = CarPhysicsConfig::getInstance().getCarPhysics(car_type);
    it->second.car.car_name = car_type;

    it->second.car.speed = phys.max_speed;
    it->second.car.acceleration = phys.max_acceleration;
    it->second.car.hp = phys.max_hp;
    it->second.car.durability = phys.collision_damage_multiplier;
    it->second.car.handling = phys.torque;

    b2Body *oldBody = it->second.body;
    if (oldBody)
    {
        b2World *world = oldBody->GetWorld();
        b2Vec2 prevPos = oldBody->GetPosition();
        float prevAngle = oldBody->GetAngle();
        b2Vec2 prevLinearVel = oldBody->GetLinearVelocity();
        float prevAngularVel = oldBody->GetAngularVelocity();
        world->DestroyBody(oldBody);

        b2BodyDef bd;
        bd.type = b2_dynamicBody;
        bd.position = prevPos;
        bd.angle = prevAngle;
        b2Body *newBody = world->CreateBody(&bd);

        const float SCALE_LOCAL = 32.0f;
        float halfW = phys.width / (2.0f * SCALE_LOCAL);
        float halfH = phys.height / (2.0f * SCALE_LOCAL);

        b2Vec2 center_offset(0.0f, phys.center_offset_y / SCALE_LOCAL);
        b2PolygonShape shape;
        shape.SetAsBox(halfW, halfH, center_offset, 0.0f);
        b2FixtureDef fd;
        fd.shape = &shape;
        fd.density = phys.density;
        fd.friction = phys.friction;
        fd.restitution = phys.restitution;

        fd.filter.categoryBits = CAR_GROUND;

        // Con lo que un auto colisiona
        fd.filter.maskBits =
            COLLISION_FLOOR |
            CAR_GROUND |
            SENSOR_START_BRIDGE |
            SENSOR_END_BRIDGE;

        newBody->CreateFixture(&fd);
        newBody->SetBullet(true);
        newBody->SetLinearDamping(phys.linear_damping);
        newBody->SetAngularDamping(phys.angular_damping);
        newBody->SetLinearVelocity(prevLinearVel);
        newBody->SetAngularVelocity(prevAngularVel);

        it->second.body = newBody;
    }
}
void GameEventHandler::handle_event(Event &event)
{
    auto it = listeners.find(event.action);
    if (it != listeners.end())
    {
        it->second(event);
    }
}

void GameEventHandler::upgrade_max_speed(Event &event)
{
    if (current_state != GameState::STARTING)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    int rounds = it->second.rounds_completed;
    if (it == players.end() || rounds == 0 || it->second.upgrades.speed >= MAX_UPGRADES_PER_STAT)
    {
        return;
    }

    it->second.car.speed *= SPEED_UPGRADE_MULTIPLIER;
    it->second.upgrades.speed++;
    uint32_t old_time = it->second.round_times_ms[rounds];

    uint64_t penalization = uint64_t(PENALIZATION_TIME) + uint64_t(old_time);

    it->second.round_times_ms[rounds] = uint32_t(penalization);

    it->second.total_time_ms = it->second.total_time_ms - old_time + uint32_t(penalization);
}

void GameEventHandler::upgrade_max_acceleration(Event &event)
{
    if (current_state != GameState::STARTING)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    int rounds = it->second.rounds_completed;
    if (it == players.end() || rounds == 0 || it->second.upgrades.acceleration >= MAX_UPGRADES_PER_STAT)
    {
        return;
    }
    it->second.car.acceleration *= ACCELERATION_UPGRADE_MULTIPLIER;
    it->second.upgrades.acceleration++;
    uint32_t old_time = it->second.round_times_ms[rounds];
    uint64_t penalization = uint64_t(PENALIZATION_TIME) + uint64_t(old_time);
    it->second.round_times_ms[rounds] = uint32_t(penalization);

    it->second.total_time_ms = it->second.total_time_ms - old_time + uint32_t(penalization);
}

void GameEventHandler::upgrade_durability(Event &event)
{
    if (current_state != GameState::STARTING)
        return;

    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    int rounds = it->second.rounds_completed;
    if (it == players.end() || rounds == 0 || it->second.upgrades.durability >= MAX_UPGRADES_PER_STAT)
    {
        return;
    }

    it->second.car.durability -= DURABILITY_UPGRADE_REDUCTION;
    it->second.upgrades.durability++;

    uint32_t old_time = it->second.round_times_ms[rounds];
    uint64_t penalization = uint64_t(PENALIZATION_TIME) + uint64_t(old_time);
    it->second.round_times_ms[rounds] = uint32_t(penalization);
    it->second.total_time_ms = it->second.total_time_ms - old_time + uint32_t(penalization);
}

void GameEventHandler::upgrade_handling(Event &event)
{

    if (current_state != GameState::STARTING)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    int rounds = it->second.rounds_completed;
    if (it == players.end() || rounds == 0 || it->second.upgrades.handling >= MAX_UPGRADES_PER_STAT)
    {
        return;
    }
    it->second.car.handling *= HANDLING_UPGRADE_MULTIPLIER;
    it->second.upgrades.handling++;

    uint32_t old_time = it->second.round_times_ms[rounds];
    uint64_t penalization = uint64_t(PENALIZATION_TIME) + uint64_t(old_time);
    it->second.round_times_ms[rounds] = uint32_t(penalization);

    it->second.total_time_ms = it->second.total_time_ms - old_time + uint32_t(penalization);
}

void GameEventHandler::cheat_god_mode(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
    {
        return;
    }

    it->second.god_mode = !it->second.god_mode;
}

void GameEventHandler::cheat_die(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
    {
        return;
    }

    it->second.pending_disqualification = true;
    it->second.god_mode = false;
}

void GameEventHandler::cheat_skip_round(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
    {
        return;
    }
    it->second.pending_race_complete = true;
}

void GameEventHandler::cheat_full_upgrade(Event &event)
{
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
    {
        return;
    }

    while (it->second.upgrades.speed < MAX_UPGRADES_PER_STAT)
    {
        it->second.car.speed *= SPEED_UPGRADE_MULTIPLIER;
        it->second.upgrades.speed++;
    }
    while (it->second.upgrades.acceleration < MAX_UPGRADES_PER_STAT)
    {
        it->second.car.acceleration *= ACCELERATION_UPGRADE_MULTIPLIER;
        it->second.upgrades.acceleration++;
    }
    while (it->second.upgrades.handling < MAX_UPGRADES_PER_STAT)
    {
        it->second.car.handling *= HANDLING_UPGRADE_MULTIPLIER;
        it->second.upgrades.handling++;
    }
    while (it->second.upgrades.durability < MAX_UPGRADES_PER_STAT)
    {
        it->second.car.durability -= DURABILITY_UPGRADE_REDUCTION;
        it->second.upgrades.durability++;
    }
}
