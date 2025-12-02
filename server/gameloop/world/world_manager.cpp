#include "world_manager.h"
#include <iostream>

void WorldManager::ContactListener::set_callback(ContactCallback cb)
{
    callback = std::move(cb);
}

void WorldManager::ContactListener::BeginContact(b2Contact *contact)
{
    if (!callback)
        return;

    b2Fixture *fixture_a = contact->GetFixtureA();
    b2Fixture *fixture_b = contact->GetFixtureB();

    callback(fixture_a, fixture_b);
}

WorldManager::WorldManager(CarPhysicsConfig &config)
    : world(b2Vec2(0.0f, 0.0f)), physics_config(config)
{
    world.SetContactListener(&contact_listener);
}

void WorldManager::set_contact_callback(ContactCallback callback)
{
    contact_listener.set_callback(std::move(callback));
}

void WorldManager::step(float time_step, int velocity_iters, int position_iters)
{
    world.Step(time_step, velocity_iters, position_iters);
}

bool WorldManager::is_locked() const
{
    return world.IsLocked();
}

b2Body *WorldManager::create_player_body(float x_px, float y_px, float angle, const std::string &car_name)
{
    const CarPhysics &car_physics = physics_config.getCarPhysics(car_name);

    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position.Set(x_px / SCALE, y_px / SCALE);
    bd.angle = angle;

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

void WorldManager::safe_destroy_body(b2Body *&body)
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
        std::cerr << "[WorldManager] Error destroying body: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "[WorldManager] Unknown error destroying body" << std::endl;
    }
}

b2World &WorldManager::get_world()
{
    return world;
}
