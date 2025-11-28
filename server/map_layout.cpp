#include "map_layout.h"
#include <fstream>
#include <iostream>
#include "../common/constants.h"
#define MAP_WIDTH (4640.0f / 32.0f)
#define MAP_HEIGHT (4672.0f / 32.0f)
#define SCALE 32.0f
#define WALL_THICKNESS 0.1f
#define OFFSET_X -10.0f
#define OFFSET_Y -10.0f
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void MapLayout::create_map_layout(const std::string &jsonPath)
{
    std::ifstream file(jsonPath);
    json data;
    file >> data;

    for (auto &layer : data[LAYERS_STR])
    {
        if (layer[TYPE_STR] != OBJECTGROUP_STR)
        {
            continue;
        }

        uint16_t category = 0;
        if (layer[NAME_STR] == LAYER_COLLISIONS_STR)
        {
            category = COLLISION_FLOOR;
        }
        if (layer[NAME_STR] == LAYER_COLLISIONS_BRIDGE_STR)
        {
            category = COLLISION_BRIDGE;
        }
        if (layer[NAME_STR] == LAYER_END_BRIDGE_STR)
        {
            category = SENSOR_END_BRIDGE;
        }
        if (layer[NAME_STR] == LAYER_COLLISIONS_UNDER_STR)
        {
            category = COLLISION_UNDER; // cosas que no son puente pero estan por arriba
        }
        if (layer[NAME_STR] == LAYER_START_BRIDGE_STR)
        {
            category = SENSOR_START_BRIDGE;
        }

        if (category == 0)
        {
            continue;
        }
        for (auto &obj : layer[OBJECTS_STR])
        {

            if (obj.contains(POLYGON_STR))
            {
                float baseX = obj[X_STR].get<float>();
                float baseY = obj[Y_STR].get<float>();

                std::vector<b2Vec2> verts;

                for (auto &pt : obj[POLYGON_STR])
                {
                    float raw_px = baseX + pt[X_STR].get<float>();
                    float raw_py = baseY + pt[Y_STR].get<float>();
                    float px = raw_px + OFFSET_X;
                    float py = raw_py + OFFSET_Y;

                    verts.emplace_back(px / SCALE, py / SCALE);
                }
                create_polygon_layout(verts, category);
                continue;
            }

            if (obj.contains(WIDTH_STR) && obj.contains(HEIGHT_STR))
            {
                float x = obj[X_STR].get<float>();
                float y = obj[Y_STR].get<float>();
                float w = obj[WIDTH_STR].get<float>();
                float h = obj[HEIGHT_STR].get<float>();
                std::vector<b2Vec2> verts;
                verts.emplace_back((x + OFFSET_X) / SCALE, (y + OFFSET_Y) / SCALE);
                verts.emplace_back((x + w + OFFSET_X) / SCALE, (y + OFFSET_Y) / SCALE);
                verts.emplace_back((x + w + OFFSET_X) / SCALE, (y + h + OFFSET_Y) / SCALE);
                verts.emplace_back((x + OFFSET_X) / SCALE, (y + h + OFFSET_Y) / SCALE);

                create_polygon_layout(verts, category);
            }
        }
    }
}

void MapLayout::extract_checkpoints(const std::string &jsonPath, std::vector<b2Vec2> &out)
{
    std::ifstream file(jsonPath);
    if (!file)
    {
        std::cerr << "[MapLayout] Could not open JSON: " << jsonPath << std::endl;
        return;
    }
    json data;
    file >> data;

    if (!data.contains(CHECKPOINTS_STR) || !data[CHECKPOINTS_STR].is_array())
    {
        std::cerr << "[MapLayout] extract_checkpoints: missing or invalid 'checkpoints' array in " << jsonPath << std::endl;
        return;
    }

    for (auto &obj : data[CHECKPOINTS_STR])
    {
        if (!obj.contains(X_STR) || !obj.contains(Y_STR))
            continue;
        float raw_x = obj[X_STR].get<float>();
        float raw_y = obj[Y_STR].get<float>();
        float px = raw_x + OFFSET_X;
        float py = raw_y + OFFSET_Y;
        out.emplace_back(px / SCALE, py / SCALE);
    }
}

void MapLayout::extract_map_npc_data(const std::string &json_path_wp, const std::string &json_path_parked, std::vector<WaypointData> &npc_waypoints, std::vector<ParkedCarData> &parked_cars)
{
    npc_waypoints.clear();
    parked_cars.clear();

    get_npc_waypoints(json_path_wp, npc_waypoints);
    get_parked_cars(json_path_parked, parked_cars);
}
void MapLayout::get_parked_cars(const std::string &json_path_parked, std::vector<ParkedCarData> &parked_cars)
{
    std::ifstream file_parked(json_path_parked);
    if (!file_parked)
    {
        throw std::runtime_error("No se pudo abrir " + json_path_parked);
    }
    nlohmann::json p;
    file_parked >> p;
    if (p.contains(PARKED_CARS_STR))
    {
        for (auto &item : p[PARKED_CARS_STR])
        {
            ParkedCarData pc;

            float px = (item[X_STR].get<float>() + OFFSET_X) / SCALE;
            float py = (item[Y_STR].get<float>() + OFFSET_Y) / SCALE;

            pc.position.Set(px, py);
            pc.horizontal = item[HORIZONTAL_STR].get<bool>();

            parked_cars.push_back(std::move(pc));
        }
    }
}

void MapLayout::get_npc_waypoints(const std::string &json_path_wp, std::vector<WaypointData> &npc_waypoints)
{
    std::ifstream file(json_path_wp);
    if (!file)
    {
        throw std::runtime_error("No se pudo abrir " + json_path_wp);
    }
    nlohmann::json j;
    file >> j;
    if (j.contains(WAYPOINTS_STR))
    {
        for (auto &obj : j[WAYPOINTS_STR])
        {
            WaypointData wp;

            float px = (obj[X_STR].get<float>() + OFFSET_X) / SCALE;
            float py = (obj[Y_STR].get<float>() + OFFSET_Y) / SCALE;
            wp.position = b2Vec2(px, py);

            if (obj.contains(CONNECTIONS_STR))
            {
                for (auto &c : obj[CONNECTIONS_STR])
                    wp.connections.push_back(c.get<int>());
            }

            npc_waypoints.push_back(std::move(wp));
        }
    }
}

void MapLayout::create_polygon_layout(const std::vector<b2Vec2> &vertices, uint16_t category)
{
    if (vertices.size() < 3)
        return;

    b2BodyDef bd;
    bd.type = b2_staticBody;
    b2Body *body = world.CreateBody(&bd);

    b2PolygonShape shape;
    shape.Set(vertices.data(), (int)vertices.size());

    b2FixtureDef fd;
    fd.shape = &shape;
    fd.density = 0.0f;
    fd.friction = 0.7f;

    fd.filter.categoryBits = category;

    if (category == COLLISION_UNDER)
    {
        fd.filter.maskBits =
            SENSOR_START_BRIDGE |
            SENSOR_END_BRIDGE |
            COLLISION_UNDER;
    }
    else if (category == COLLISION_BRIDGE)
    {
        fd.filter.maskBits =
            CAR_BRIDGE |
            SENSOR_START_BRIDGE |
            SENSOR_END_BRIDGE;
    }
    else if (category == COLLISION_FLOOR)
    {
        fd.filter.maskBits =
            CAR_GROUND |
            SENSOR_START_BRIDGE |
            SENSOR_END_BRIDGE;
    }

    if (category == SENSOR_START_BRIDGE || category == SENSOR_END_BRIDGE)
    {
        fd.isSensor = true;

        fd.filter.maskBits =
            CAR_GROUND |
            CAR_BRIDGE |
            COLLISION_FLOOR |
            COLLISION_BRIDGE |
            COLLISION_UNDER;
    }

    body->CreateFixture(&fd);
}