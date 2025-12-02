#include "bridge_handler.h"
#include <iostream>

bool BridgeHandler::update_bridge_state(PlayerData &player_data)
{
    if (!player_data.body)
    {
        return player_data.position.on_bridge;
    }

    BridgeContactResult result = analyze_bridge_contacts(player_data.body);

    // Actualizar el estado del puente
    if (result.entering_bridge && !player_data.position.on_bridge)
    {
        std::cout << "[BRIDGE] Player entering bridge (CAR_GROUND -> CAR_BRIDGE)" << std::endl;
        set_collision_category(player_data, CAR_BRIDGE);
        player_data.position.on_bridge = true;
    }
    else if (result.leaving_bridge && player_data.position.on_bridge)
    {
        std::cout << "[BRIDGE] Player leaving bridge (CAR_BRIDGE -> CAR_GROUND)" << std::endl;
        set_collision_category(player_data, CAR_GROUND);
        player_data.position.on_bridge = false;
    }

    return player_data.position.on_bridge;
}

void BridgeHandler::update_bridge_state(NPCData &npc_data)
{
    if (!npc_data.body)
    {
        return;
    }

    BridgeContactResult result = analyze_bridge_contacts(npc_data.body);

    // Actualizar el estado del puente
    if (result.entering_bridge && !npc_data.on_bridge)
    {
        std::cout << "[BRIDGE] NPC entering bridge (CAR_GROUND -> CAR_BRIDGE)" << std::endl;
        set_collision_category(npc_data, CAR_BRIDGE);
        npc_data.on_bridge = true;
    }
    else if (result.leaving_bridge && npc_data.on_bridge)
    {
        std::cout << "[BRIDGE] NPC leaving bridge (CAR_BRIDGE -> CAR_GROUND)" << std::endl;
        set_collision_category(npc_data, CAR_GROUND);
        npc_data.on_bridge = false;
    }
}

BridgeHandler::BridgeContactResult BridgeHandler::analyze_bridge_contacts(b2Body *body)
{
    BridgeContactResult result;

    b2ContactEdge *ce = body->GetContactList();
    while (ce)
    {
        b2Contact *c = ce->contact;

        if (c->IsTouching())
        {
            b2Fixture *a = c->GetFixtureA();
            b2Fixture *b = c->GetFixtureB();

            uint16 fcA = a->GetFilterData().categoryBits;
            uint16 fcB = b->GetFilterData().categoryBits;

            // Detectar entrada al puente
            if (fcA == SENSOR_START_BRIDGE || fcB == SENSOR_START_BRIDGE)
            {
                result.entering_bridge = true;
            }

            // Detectar salida del puente
            if (fcA == SENSOR_END_BRIDGE || fcB == SENSOR_END_BRIDGE)
            {
                result.leaving_bridge = true;
            }
        }

        ce = ce->next;
    }

    return result;
}

void BridgeHandler::set_collision_category(PlayerData &player_data, uint16 new_category)
{
    b2Body *body = player_data.body;
    if (!body)
        return;

    for (b2Fixture *f = body->GetFixtureList(); f; f = f->GetNext())
    {
        apply_category_filter(f, new_category);
    }
}

void BridgeHandler::set_collision_category(NPCData &npc_data, uint16 new_category)
{
    b2Body *body = npc_data.body;
    if (!body)
        return;

    for (b2Fixture *f = body->GetFixtureList(); f; f = f->GetNext())
    {
        apply_category_filter(f, new_category);
    }
}

void BridgeHandler::apply_category_filter(b2Fixture *fixture, uint16 new_category)
{
    b2Filter filter = fixture->GetFilterData();
    filter.categoryBits = new_category;

    if (new_category == CAR_GROUND)
    {
        // Auto en el SUELO: ve suelo, otros autos en suelo, y sensores
        filter.maskBits =
            COLLISION_FLOOR |
            CAR_GROUND |
            SENSOR_START_BRIDGE |
            SENSOR_END_BRIDGE;
    }
    else if (new_category == CAR_BRIDGE)
    {
        // Auto EN EL PUENTE: ve puente, cosas sobre puente, otros autos en puente, y sensores
        filter.maskBits =
            COLLISION_BRIDGE |
            COLLISION_UNDER |
            COLLISION_FLOOR |
            CAR_BRIDGE |
            SENSOR_START_BRIDGE |
            SENSOR_END_BRIDGE;
    }

    fixture->SetFilterData(filter);
}
