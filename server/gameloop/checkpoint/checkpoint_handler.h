#ifndef CHECKPOINT_HANDLER_H
#define CHECKPOINT_HANDLER_H

#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_world.h>
#include <box2d/b2_circle_shape.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <array>
#include "../../PlayerData.h"
#include "../../map_layout.h"

class CheckpointHandler
{
public:
    static constexpr float SCALE = 32.0f;
    // Configura los checkpoints desde un archivo JSON
    static void setup_checkpoints_from_file(
        const std::string &json_path,
        b2World &world,
        MapLayout &map_layout,
        std::vector<b2Vec2> &checkpoint_centers,
        std::unordered_map<b2Fixture *, int> &checkpoint_fixtures);

    // Limpia y recarga los checkpoints para una nueva ronda
    static void load_round_checkpoints(
        int current_round,
        const std::array<std::string, 3> &checkpoint_sets,
        b2World &world,
        MapLayout &map_layout,
        std::vector<b2Vec2> &checkpoint_centers,
        std::unordered_map<b2Fixture *, int> &checkpoint_fixtures);

    // Valida si una colisión es un checkpoint válido
    static bool is_valid_checkpoint_collision(
        b2Fixture *player_fixture,
        b2Fixture *checkpoint_fixture,
        const std::unordered_map<int, PlayerData> &players,
        const std::unordered_map<b2Fixture *, int> &checkpoint_fixtures,
        int &out_player_id,
        int &out_checkpoint_index);

    // Maneja cuando un jugador alcanza un checkpoint
    // Retorna true si el jugador completó la vuelta
    static bool handle_checkpoint_reached(
        PlayerData &player_data,
        int player_id,
        int checkpoint_index,
        int total_checkpoints);

private:
    // Encuentra el ID del jugador por su body de Box2D
    static int find_player_by_body(
        b2Body *body,
        const std::unordered_map<int, PlayerData> &players);
};

#endif
