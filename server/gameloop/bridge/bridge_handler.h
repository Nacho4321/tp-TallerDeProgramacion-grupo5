#ifndef BRIDGE_HANDLER_H
#define BRIDGE_HANDLER_H

#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_contact.h>
#include "../../PlayerData.h"
#include "../npc/npc_data.h"
#include "../gameloop_constants.h"


// BridgeHandler - Maneja la lógica de transición de vehículos entre suelo y puente.
class BridgeHandler
{
public:
    static bool update_bridge_state(PlayerData &player_data);
    static void update_bridge_state(NPCData &npc_data);

private:
    struct BridgeContactResult
    {
        bool entering_bridge{false};
        bool leaving_bridge{false};
    };
    static BridgeContactResult analyze_bridge_contacts(b2Body *body);
    static void set_collision_category(PlayerData &player_data, uint16 new_category);
    static void set_collision_category(NPCData &npc_data, uint16 new_category);
    static void apply_category_filter(b2Fixture *fixture, uint16 new_category);
};

#endif 
