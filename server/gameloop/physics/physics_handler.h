#ifndef PHYSICS_HANDLER_H
#define PHYSICS_HANDLER_H

#include <box2d/b2_body.h>
#include <box2d/b2_math.h>
#include "../../PlayerData.h"
#include "../../car_physics_config.h"

class PhysicsHandler
{
public:
    // Constantes de física
    static constexpr float SCALE = 32.0f;
    static constexpr float RIGHT_VECTOR_X = 1.0f;
    static constexpr float RIGHT_VECTOR_Y = 0.0f;
    static constexpr float FORWARD_VECTOR_X = 0.0f;
    static constexpr float FORWARD_VECTOR_Y = 1.0f;

    // Obtener componentes de velocidad
    static b2Vec2 get_lateral_velocity(b2Body *body);
    static b2Vec2 get_forward_velocity(b2Body *body);

    // Aplicar física al jugador
    static void update_friction_for_player(PlayerData &player_data, CarPhysicsConfig &physics_config);
    static void update_drive_for_player(PlayerData &player_data, CarPhysicsConfig &physics_config);

    // Utilidades
    static float normalize_angle(double angle);

private:
    // Helpers internos
    static float calculate_desired_speed(bool want_up, bool want_down, const CarPhysics &car_physics);
    static void apply_forward_drive_force(b2Body *body, float desired_speed, const CarPhysics &car_physics);
    static void apply_steering_torque(b2Body *body, bool want_left, bool want_right, float torque);
};

#endif
