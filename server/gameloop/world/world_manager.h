#ifndef WORLD_MANAGER_H
#define WORLD_MANAGER_H

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_contact.h>
#include <string>
#include <functional>
#include "../../car_physics_config.h"
#include "../../../common/constants.h"

// Callback para cuando hay un contacto
using ContactCallback = std::function<void(b2Fixture *, b2Fixture *)>;

class WorldManager
{
public:
    // Contact listener interno que delega al callback
    class ContactListener : public b2ContactListener
    {
    private:
        ContactCallback callback;

    public:
        void set_callback(ContactCallback cb);
        void BeginContact(b2Contact *contact) override;
    };

private:
    b2World world;
    ContactListener contact_listener;
    CarPhysicsConfig &physics_config;

    static constexpr float SCALE = 32.0f;

public:
    explicit WorldManager(CarPhysicsConfig &config);

    // Configurar el callback de contacto
    void set_contact_callback(ContactCallback callback);

    // Avanzar la simulación física
    void step(float time_step, int velocity_iters, int position_iters);

    // Verificar si el mundo está bloqueado (en medio de un step)
    bool is_locked() const;

    // Crear un body para un jugador
    b2Body *create_player_body(float x_px, float y_px, float angle, const std::string &car_name);

    // Destruir un body de forma segura
    void safe_destroy_body(b2Body *&body);

    // Acceso al mundo para componentes que lo necesiten (MapLayout, NPCManager)
    b2World &get_world();
};

#endif
