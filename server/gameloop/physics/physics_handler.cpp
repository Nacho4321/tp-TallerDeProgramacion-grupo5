#include "physics_handler.h"
#include "../../event.h"
#include <cmath>
#include <iostream>

b2Vec2 PhysicsHandler::get_lateral_velocity(b2Body *body)
{
    b2Vec2 currentRightNormal = body->GetWorldVector(b2Vec2(RIGHT_VECTOR_X, RIGHT_VECTOR_Y));
    return b2Dot(currentRightNormal, body->GetLinearVelocity()) * currentRightNormal;
}

b2Vec2 PhysicsHandler::get_forward_velocity(b2Body *body)
{
    b2Vec2 currentForwardNormal = body->GetWorldVector(b2Vec2(FORWARD_VECTOR_X, FORWARD_VECTOR_Y));
    return b2Dot(currentForwardNormal, body->GetLinearVelocity()) * currentForwardNormal;
}

void PhysicsHandler::update_friction_for_player(PlayerData &player_data, CarPhysicsConfig &physics_config)
{
    b2Body *body = player_data.body;
    if (!body)
        return;

    const CarPhysics &car_physics = physics_config.getCarPhysics(player_data.car.car_name);

    // Impulso lateral para reducir el deslizamiento lateral (limitado para permitir derrapes)
    b2Vec2 impulse = body->GetMass() * -get_lateral_velocity(body);
    float ilen = impulse.Length();
    float maxImpulse = car_physics.max_lateral_impulse * body->GetMass();
    if (ilen > maxImpulse)
        impulse *= maxImpulse / ilen;
    body->ApplyLinearImpulse(impulse, body->GetWorldCenter(), true);

    // Matar un poco la velocidad angular para evitar giros descontrolados
    body->ApplyAngularImpulse(car_physics.angular_friction * body->GetInertia() * -body->GetAngularVelocity(), true);

    b2Vec2 forwardDir = body->GetWorldVector(b2Vec2(0, 1));
    float currentForwardSpeed = b2Dot(body->GetLinearVelocity(), forwardDir);
    b2Vec2 dragForce = body->GetMass() * car_physics.forward_drag_coefficient * currentForwardSpeed * forwardDir;
    body->ApplyForce(dragForce, body->GetWorldCenter(), true);
}

float PhysicsHandler::calculate_desired_speed(bool want_up, bool want_down, const CarPhysics &car_physics)
{
    if (want_up)
        return car_physics.max_speed / SCALE;
    if (want_down)
        return -car_physics.max_speed * car_physics.backward_speed_multiplier / SCALE;
    return 0.0f;
}

void PhysicsHandler::apply_forward_drive_force(b2Body *body, float desired_speed, const CarPhysics &car_physics)
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

void PhysicsHandler::apply_steering_torque(b2Body *body, bool want_left, bool want_right, float torque)
{
    if (want_left)
        body->ApplyTorque(-torque, true);
    else if (want_right)
        body->ApplyTorque(torque, true);
}

void PhysicsHandler::update_drive_for_player(PlayerData &player_data, CarPhysicsConfig &physics_config)
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
    if (want_down)
    {
        b2Vec2 forwardNormal = body->GetWorldVector(b2Vec2(FORWARD_VECTOR_X, FORWARD_VECTOR_Y));
        float current_speed = b2Dot(body->GetLinearVelocity(), forwardNormal);
        float threshold = 1.0f;
        if (current_speed > threshold)
        {
            player_data.is_stopping = true;
            std::cout << "[FRENAZO] Player frenazo Velocidad: " << current_speed << std::endl;
        }
    }

    if (want_up || want_down)
    {
        float desired_speed = calculate_desired_speed(want_up, want_down, car_physics);
        apply_forward_drive_force(body, desired_speed, car_physics);
    }

    apply_steering_torque(body, want_left, want_right, car_physics.torque);
}

float PhysicsHandler::normalize_angle(double angle)
{
    while (angle < 0.0)
        angle += 2.0 * M_PI;
    while (angle >= 2.0 * M_PI)
        angle -= 2.0 * M_PI;
    return static_cast<float>(angle);
}
