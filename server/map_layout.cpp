#include "map_layout.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include "../common/constants.h"
#define MAP_WIDTH (4640.0f / 32.0f)
#define MAP_HEIGHT (4672.0f / 32.0f)
#define SCALE 32.0f
#define WALL_THICKNESS 0.1f
#define OFFSET_X -10.0f
#define OFFSET_Y -10.0f

using json = nlohmann::json;

void MapLayout::create_map_layout(const std::string &jsonPath)
{
    std::ifstream file(jsonPath);
    json data;
    file >> data;

    for (auto &layer : data["layers"])
    {
        if (layer["type"] != "objectgroup")
            continue;

        uint16_t category = 0;
        if (layer["name"] == "Collisions")
        {
            category = COLLISION_FLOOR;
        }
        if (layer["name"] == "Collisions_Bridge")
        {
            category = COLLISION_BRIDGE;
        }
        if (layer["name"] == "End_Bridge")
        {
            category = SENSOR_END_BRIDGE;
        }
        if (layer["name"] == "Collisions_under")
        {
            category = COLLISION_UNDER; // cosas que no son puente pero estan por arriba
        }
        if (layer["name"] == "Start_Bridge")
        {
            category = SENSOR_START_BRIDGE;
        }

        if (category == 0)
        {
            continue;
        }
        for (auto &obj : layer["objects"])
        {
            // CASO 1: POLÍGONO

            if (obj.contains("polygon"))
            {
                float baseX = obj["x"].get<float>();
                float baseY = obj["y"].get<float>();

                // std::cout << "Polygon RAW coords:\n";
                std::vector<b2Vec2> verts;

                for (auto &pt : obj["polygon"])
                {
                    float raw_px = baseX + pt["x"].get<float>();
                    float raw_py = baseY + pt["y"].get<float>();

                    // std::cout << raw_px << ", " << raw_py << "\n";

                    float px = raw_px + OFFSET_X;
                    float py = raw_py + OFFSET_Y;

                    verts.emplace_back(px / SCALE, py / SCALE);
                }

                // std::cout << "----\n";

                create_polygon_layout(verts, category);
                continue;
            }

            // CASO 2: RECTÁNGULO
            if (obj.contains("width") && obj.contains("height"))
            {
                float x = obj["x"].get<float>();
                float y = obj["y"].get<float>();
                float w = obj["width"].get<float>();
                float h = obj["height"].get<float>();

                // std::cout << "Square RAW coords:\n";
                // std::cout << x << ", " << y << "\n";
                // std::cout << x + w << ", " << y << "\n";
                // std::cout << x + w << ", " << y + h << "\n";
                // std::cout << x << ", " << y + h << "\n";
                // std::cout << "----\n";

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

    if (!data.contains("checkpoints") || !data["checkpoints"].is_array())
    {
        std::cerr << "[MapLayout] extract_checkpoints: missing or invalid 'checkpoints' array in " << jsonPath << std::endl;
        return;
    }

    for (auto &obj : data["checkpoints"])
    {
        if (!obj.contains("x") || !obj.contains("y"))
            continue;
        float raw_x = obj["x"].get<float>();
        float raw_y = obj["y"].get<float>();
        float px = raw_x + OFFSET_X;
        float py = raw_y + OFFSET_Y;
        out.emplace_back(px / SCALE, py / SCALE);
    }
}

void MapLayout::extract_npc_waypoints(const std::string &jsonPath, std::vector<WaypointData> &out)
{
    std::ifstream file(jsonPath);
    if (!file)
    {
        std::cerr << "[MapLayout] Could not open JSON: " << jsonPath << std::endl;
        return;
    }
    json data;
    file >> data;

    if (!data.contains("waypoints") || !data["waypoints"].is_array())
    {
        std::cerr << "[MapLayout] extract_npc_waypoints: missing or invalid 'waypoints' array in " << jsonPath << std::endl;
        return;
    }

    for (auto &obj : data["waypoints"])
    {
        if (!obj.contains("x") || !obj.contains("y"))
            continue;

        float raw_x = obj["x"].get<float>();
        float raw_y = obj["y"].get<float>();
        float px = raw_x + OFFSET_X;
        float py = raw_y + OFFSET_Y;

        WaypointData wp;
        wp.position = b2Vec2(px / SCALE, py / SCALE);

        // Extract connections array
        if (obj.contains("connections") && obj["connections"].is_array())
        {
            for (auto &conn : obj["connections"])
            {
                wp.connections.push_back(conn.get<int>());
            }
        }

        out.push_back(wp);
    }

    std::cout << "[MapLayout] Loaded " << out.size() << " NPC waypoints from " << jsonPath << std::endl;
}

void MapLayout::extract_parked_cars(const std::string &jsonPath, std::vector<ParkedCarData> &out)
{
    out.clear();

    std::ifstream file(jsonPath);
    if (!file.is_open())
    {
        std::cerr << "[MapLayout] ERROR: Cannot open parked cars file: " << jsonPath << std::endl;
        return;
    }

    nlohmann::json j;
    file >> j;
    file.close();

    if (!j.contains("parked_cars") || !j["parked_cars"].is_array())
    {
        std::cerr << "[MapLayout] ERROR: JSON missing 'parked_cars' array" << std::endl;
        return;
    }

    for (const auto &item : j["parked_cars"])
    {
        if (!item.contains("x") || !item.contains("y") || !item.contains("horizontal"))
        {
            std::cerr << "[MapLayout] WARNING: Parked car missing x, y or horizontal field" << std::endl;
            continue;
        }

        ParkedCarData pc;
        float raw_x = item["x"].get<float>();
        float raw_y = item["y"].get<float>();

        // Aplicar offset y escala
        float x_m = (raw_x + static_cast<float>(OFFSET_X)) / static_cast<float>(SCALE);
        float y_m = (raw_y + static_cast<float>(OFFSET_Y)) / static_cast<float>(SCALE);

        pc.position.Set(x_m, y_m);
        pc.horizontal = item["horizontal"].get<bool>();

        out.push_back(pc);
    }

    std::cout << "[MapLayout] Loaded " << out.size() << " parked cars from " << jsonPath << std::endl;
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
            CAR_NPC |
            SENSOR_START_BRIDGE |
            SENSOR_END_BRIDGE;
    }

    if (category == SENSOR_START_BRIDGE || category == SENSOR_END_BRIDGE)
    {
        fd.isSensor = true;

        fd.filter.maskBits =
            CAR_GROUND |
            CAR_BRIDGE |
            CAR_NPC |
            COLLISION_FLOOR |
            COLLISION_BRIDGE |
            COLLISION_UNDER;
    }

    body->CreateFixture(&fd);
}