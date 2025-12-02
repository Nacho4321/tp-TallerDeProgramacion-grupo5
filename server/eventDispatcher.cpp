#include "eventDispatcher.h"
#include "../common/constants.h"
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_fixture.h>

void EventDispatcher::init_handlers()
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
    // Handlers de cambio de auto (prefijos change_car <tipo>)
    listeners[std::string(CHANGE_CAR_STR) + " " + GREEN_CAR] = [this](Event &e)
    { change_car(e, GREEN_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + RED_SQUARED_CAR] = [this](Event &e)
    { change_car(e, RED_SQUARED_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + RED_SPORTS_CAR] = [this](Event &e)
    { change_car(e, RED_SPORTS_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + LIGHT_BLUE_CAR] = [this](Event &e)
    { change_car(e, LIGHT_BLUE_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + RED_JEEP_CAR] = [this](Event &e)
    { change_car(e, RED_JEEP_CAR); };
    listeners[std::string(CHANGE_CAR_STR) + " " + PURPLE_TRUCK] = [this](Event &e)
    { change_car(e, PURPLE_TRUCK); };
    listeners[std::string(CHANGE_CAR_STR) + " " + LIMOUSINE_CAR] = [this](Event &e)
    { change_car(e, LIMOUSINE_CAR); };
    
    // Handlers de upgrade (durante STARTING)
    listeners[std::string(UPGRADE_CAR_STR) + " 0"] = [this](Event &e)
    { upgrade_speed(e); };
    listeners[std::string(UPGRADE_CAR_STR) + " 1"] = [this](Event &e)
    { upgrade_acceleration(e); };
    listeners[std::string(UPGRADE_CAR_STR) + " 2"] = [this](Event &e)
    { upgrade_handling(e); };
    listeners[std::string(UPGRADE_CAR_STR) + " 3"] = [this](Event &e)
    { upgrade_durability(e); };
}

void EventDispatcher::move_up(Event &event)
{
    if (current_state != GameState::PLAYING) return; // Disable movement in LOBBY/STARTING
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_y = up;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_up_released(Event &event)
{
    if (current_state != GameState::PLAYING) return; // Ignore in LOBBY/STARTING
    std::lock_guard<std::mutex> lock(players_map_mutex);
    if (players[event.client_id].state != MOVE_DOWN_PRESSED_STR)
    {
        players[event.client_id].position.direction_y = not_vertical;
        players[event.client_id].state = event.action;
    }
}

void EventDispatcher::move_down(Event &event)
{
    if (current_state != GameState::PLAYING) return;
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_y = down;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_down_released(Event &event)
{
    if (current_state != GameState::PLAYING) return;
    std::lock_guard<std::mutex> lock(players_map_mutex);
    if (players[event.client_id].state != MOVE_UP_PRESSED_STR)
    {
        players[event.client_id].position.direction_y = not_vertical;
        players[event.client_id].state = event.action;
    }
}

void EventDispatcher::move_left(Event &event)
{
    if (current_state != GameState::PLAYING) return;
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_x = left;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_left_released(Event &event)
{
    if (current_state != GameState::PLAYING) return;
    std::lock_guard<std::mutex> lock(players_map_mutex);
    if (players[event.client_id].state != MOVE_RIGHT_PRESSED_STR)
    {
        players[event.client_id].position.direction_x = not_horizontal;
        players[event.client_id].state = event.action;
    }
}

void EventDispatcher::move_right(Event &event)
{
    if (current_state != GameState::PLAYING) return;
    std::lock_guard<std::mutex> lock(players_map_mutex);
    players[event.client_id].position.direction_x = right;
    players[event.client_id].state = event.action;
}

void EventDispatcher::move_right_released(Event &event)
{
    if (current_state != GameState::PLAYING) return;
    std::lock_guard<std::mutex> lock(players_map_mutex);
    if (players[event.client_id].state != MOVE_LEFT_PRESSED_STR)
    {
        players[event.client_id].position.direction_x = not_horizontal;
        players[event.client_id].state = event.action;
    }
}
void EventDispatcher::change_car(Event &event, const std::string &car_type)
{
    if (current_state != GameState::LOBBY) {
        return;
    }
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
        return;
    const CarPhysics &phys = CarPhysicsConfig::getInstance().getCarPhysics(car_type);
    it->second.car.car_name = car_type;
    // Map physics to simple CarInfo used by drive logic
    it->second.car.speed = phys.max_speed;               // already in px/s
    it->second.car.acceleration = phys.max_acceleration; // px/s^2
    it->second.car.hp = phys.max_hp;  // Set HP from physics config
    // Re-crear el body para que el fixture (hitbox) coincida con el tamaño físico del nuevo auto.
    b2Body *oldBody = it->second.body;
    if (oldBody)
    {
        b2World *world = oldBody->GetWorld();
        b2Vec2 prevPos = oldBody->GetPosition();
        float prevAngle = oldBody->GetAngle();
        b2Vec2 prevLinearVel = oldBody->GetLinearVelocity();
        float prevAngularVel = oldBody->GetAngularVelocity();
        // Destruir body anterior
        world->DestroyBody(oldBody);

        // Crear nuevo body con dimensiones del nuevo auto
        b2BodyDef bd;
        bd.type = b2_dynamicBody;
        bd.position = prevPos;
        bd.angle = prevAngle;
        b2Body *newBody = world->CreateBody(&bd);
        // Convertir de pixeles a metros (SCALE fijo = 32.0f usado en gameloop)
        const float SCALE_LOCAL = 32.0f;
        float halfW = phys.width / (2.0f * SCALE_LOCAL);
        float halfH = phys.height / (2.0f * SCALE_LOCAL);
        // Offset del centro de colisión en coordenadas locales (Y positivo = hacia adelante del auto)
        b2Vec2 center_offset(0.0f, phys.center_offset_y / SCALE_LOCAL);
        b2PolygonShape shape;
        shape.SetAsBox(halfW, halfH, center_offset, 0.0f); // último parámetro es ángulo (0 = sin rotación)
        b2FixtureDef fd;
        fd.shape = &shape;
        fd.density = phys.density;
        fd.friction = phys.friction;
        fd.restitution = phys.restitution;
        // Configurar máscaras de colisión para que NO colisione con puentes ni objetos superiores
        fd.filter.categoryBits = CAR_GROUND;
        fd.filter.maskBits =
            COLLISION_FLOOR |     // Colisiones del suelo
            CAR_GROUND |          // Otros jugadores en el suelo
            SENSOR_START_BRIDGE | // Sensores de entrada
            SENSOR_END_BRIDGE;    // Sensores de salida
        newBody->CreateFixture(&fd);
        newBody->SetBullet(true);
        newBody->SetLinearDamping(phys.linear_damping);
        newBody->SetAngularDamping(phys.angular_damping);
        newBody->SetLinearVelocity(prevLinearVel);
        newBody->SetAngularVelocity(prevAngularVel);
        it->second.body = newBody;
    }
    std::cout << "[EventDispatcher] Player " << event.client_id << " cambió a auto '" << car_type << "' speed=" << phys.max_speed << " accel=" << phys.max_acceleration
              << " (hitbox reconstruida: " << phys.width << "x" << phys.height << ")" << std::endl;
}
void EventDispatcher::handle_event(Event &event)
{
    auto it = listeners.find(event.action);
    if (it != listeners.end())
    {
        it->second(event);
    }
    else
    {
        std::cout << "Evento desconocido: " << event.action << std::endl;
    }
}

void EventDispatcher::upgrade_speed(Event &event)
{
    if (current_state != GameState::STARTING)
        return;
    
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
        return;
    
    if (it->second.upgrades.speed >= MAX_UPGRADE_LEVEL)
        return;
    
    (void)CarPhysicsConfig::getInstance().getCarPhysics(it->second.car.car_name);
    it->second.car.speed *= SPEED_UPGRADE_MULTIPLIER;
    it->second.upgrades.speed++;
    
    std::cout << "[EventDispatcher] Player " << event.client_id 
              << " upgrade_speed nivel " << static_cast<int>(it->second.upgrades.speed)
              << " -> speed=" << it->second.car.speed << std::endl;
}

void EventDispatcher::upgrade_acceleration(Event &event)
{
    if (current_state != GameState::STARTING)
        return;
    
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
        return;
    
    if (it->second.upgrades.acceleration >= MAX_UPGRADE_LEVEL)
        return;
    
    it->second.car.acceleration *= ACCELERATION_UPGRADE_MULTIPLIER;
    it->second.upgrades.acceleration++;
    
    std::cout << "[EventDispatcher] Player " << event.client_id 
              << " upgrade_acceleration nivel " << static_cast<int>(it->second.upgrades.acceleration)
              << " -> acceleration=" << it->second.car.acceleration << std::endl;
}

void EventDispatcher::upgrade_handling(Event &event)
{
    if (current_state != GameState::STARTING)
        return;
    
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
        return;
    
    if (it->second.upgrades.handling >= MAX_UPGRADE_LEVEL)
        return;
    
    it->second.car.handling *= HANDLING_UPGRADE_MULTIPLIER;
    it->second.upgrades.handling++;
    
    std::cout << "[EventDispatcher] Player " << event.client_id 
              << " upgrade_handling nivel " << static_cast<int>(it->second.upgrades.handling)
              << " -> handling=" << it->second.car.handling << std::endl;
}

void EventDispatcher::upgrade_durability(Event &event)
{
    if (current_state != GameState::STARTING)
        return;
    
    std::lock_guard<std::mutex> lock(players_map_mutex);
    auto it = players.find(event.client_id);
    if (it == players.end())
        return;
    
    if (it->second.upgrades.durability >= MAX_UPGRADE_LEVEL)
        return;
    
    // Reduce el multiplicador de daño (menos daño recibido)
    it->second.car.durability *= (1.0f - DURABILITY_UPGRADE_REDUCTION);
    it->second.upgrades.durability++;
    
    std::cout << "[EventDispatcher] Player " << event.client_id 
              << " upgrade_durability nivel " << static_cast<int>(it->second.upgrades.durability)
              << " -> durability=" << it->second.car.durability << std::endl;
}