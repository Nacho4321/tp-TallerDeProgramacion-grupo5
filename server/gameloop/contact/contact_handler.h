#ifndef CONTACT_HANDLER_H
#define CONTACT_HANDLER_H

#include <mutex>
#include <unordered_map>
#include <atomic>
#include <functional>
#include <box2d/b2_fixture.h>
#include "../../PlayerData.h"
#include "../../game_state.h"
#include "../checkpoint/checkpoint_handler.h"
#include "../collision/collision_handler.h"
#include "../race/race_manager.h"

class ContactHandler
{
public:
    ContactHandler(
        std::mutex &players_map_mutex,
        std::unordered_map<int, PlayerData> &players,
        std::unordered_map<b2Fixture *, int> &checkpoint_fixtures,
        std::vector<b2Vec2> &checkpoint_centers,
        std::atomic<bool> &pending_race_reset,
        std::function<GameState()> get_state);

    // Main contact handler - llamado por Box2D
    void handle_begin_contact(b2Fixture *fixture_a, b2Fixture *fixture_b);

private:
    void process_pair(b2Fixture *maybePlayerFix, b2Fixture *maybeCheckpointFix);

    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    std::unordered_map<b2Fixture *, int> &checkpoint_fixtures;
    std::vector<b2Vec2> &checkpoint_centers;
    std::atomic<bool> &pending_race_reset;
    std::function<GameState()> get_state;
};

#endif

