#include "map_layout.h"
#include <fstream>
#include <iostream>
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
        auto layer_category = collisions_byte_map.find(layer[NAME_STR]);
        if (layer_category->first.empty())
        {
            continue;
        }
        uint16_t category = layer_category->second;
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

void MapLayout::extract_map_npc_data(const std::string &json_path, std::vector<WaypointData> &npc_waypoints, std::vector<ParkedCarData> &parked_cars)
{
    npc_waypoints.clear();
    parked_cars.clear();

    get_npc_waypoints(json_path, npc_waypoints);
    get_parked_cars(json_path, parked_cars);
}
void MapLayout::get_parked_cars(const std::string &json_path_parked, std::vector<ParkedCarData> &parked_cars)
{
    std::ifstream file_parked(json_path_parked);
    if (!file_parked)
    {
        throw std::runtime_error("No se pudo abrir " + json_path_parked);
    }
    nlohmann::json root;
    file_parked >> root;

    if (root.contains(LAYERS_STR))
    {
        for (auto &layer : root[LAYERS_STR])
        {
            if (layer.contains(NAME_STR) && layer[NAME_STR] == PARKED_CARS_STR)
            {
                if (layer.contains(OBJECTS_STR))
                {
                    for (auto &item : layer[OBJECTS_STR])
                    {
                        ParkedCarData pc;

                        float px = (item[X_STR].get<float>() + OFFSET_X) / SCALE;
                        float py = (item[Y_STR].get<float>() + OFFSET_Y) / SCALE;

                        pc.position.Set(px, py);

                        pc.horizontal = false;
                        if (item.contains("properties") && !item["properties"].empty())
                        {
                            for (auto &prop : item["properties"])
                            {
                                if (prop.contains(NAME_STR) && prop[NAME_STR] == HORIZONTAL_STR)
                                {
                                    pc.horizontal = prop["value"].get<bool>();
                                    break;
                                }
                            }
                        }

                        parked_cars.push_back(std::move(pc));
                    }
                }
                break;
            }
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
    nlohmann::json root;
    file >> root;

    std::vector<std::pair<int, WaypointData>> temp;

    if (root.contains(LAYERS_STR))
    {
        for (auto &layer : root[LAYERS_STR])
        {
            if (layer.contains(NAME_STR) && layer[NAME_STR] == "npc_waypoints")
            {
                if (layer.contains(OBJECTS_STR))
                {
                    for (auto &obj : layer[OBJECTS_STR])
                    {
                        WaypointData wp;
                        int id = -1;

                        float px = (obj[X_STR].get<float>() + OFFSET_X) / SCALE;
                        float py = (obj[Y_STR].get<float>() + OFFSET_Y) / SCALE;
                        wp.position = b2Vec2(px, py);

                        if (obj.contains("properties"))
                        {
                            for (auto &prop : obj["properties"])
                            {
                                if (prop[NAME_STR] == "connections")
                                {
                                    std::string conexiones = prop["value"].get<std::string>();
                                    std::vector<std::string> lista = split(conexiones, ",");
                                    for (auto &s : lista)
                                    {
                                        wp.connections.push_back(std::stoi(s));
                                    }
                                }
                                else if (prop[NAME_STR] == "id")
                                {
                                    id = prop["value"].get<int>();
                                }
                            }
                        }

                        if (id == -1)
                        {
                            std::string name = obj[NAME_STR].get<std::string>();
                            size_t pos = name.find("_");
                            if (pos != std::string::npos)
                            {
                                id = std::stoi(name.substr(pos + 1));
                            }
                        }

                        temp.push_back({id, wp});
                    }
                }
            }
        }
    }

    int max_id = 0;
    for (const auto &p : temp)
    {
        if (p.first > max_id)
            max_id = p.first;
    }
    npc_waypoints.clear();
    npc_waypoints.resize(max_id + 1);

    for (const auto &p : temp)
    {
        npc_waypoints[p.first] = p.second;
    }
}

std::vector<std::string> MapLayout::split(std::string s, const std::string &delim)
{
    std::vector<std::string> out;
    size_t pos;

    while ((pos = s.find(delim)) != std::string::npos)
    {
        std::string token = s.substr(0, pos);
        if (!token.empty())
        {
            out.push_back(token);
        }
        s.erase(0, pos + delim.length());
    }

    if (!s.empty())
    {
        out.push_back(s);
    }
    return out;
}

void MapLayout::extract_spawn_points(const std::string &jsonPath, std::vector<SpawnPointData> &out)
{
    out.clear();

    std::ifstream file(jsonPath);
    if (!file.is_open())
    {
        std::cerr << "[MapLayout] extract_spawn_points: cannot open " << jsonPath << std::endl;
        return;
    }

    nlohmann::json data;
    try
    {
        file >> data;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[MapLayout] extract_spawn_points: JSON parse error in " << jsonPath << ": " << e.what() << std::endl;
        return;
    }

    if (!data.contains("spawn_points") || !data["spawn_points"].is_array())
    {
        std::cerr << "[MapLayout] extract_spawn_points: missing 'spawn_points' array in " << jsonPath << std::endl;
        return;
    }

    for (auto &obj : data["spawn_points"])
    {
        if (!obj.contains("x") || !obj.contains("y"))
            continue;

        float x = obj["x"].get<float>();
        float y = obj["y"].get<float>();
        float angle = 0.0f;
        if (obj.contains("angle"))
            angle = obj["angle"].get<float>();

        std::string units = "pixels";
        if (obj.contains("units") && obj["units"].is_string())
            units = obj["units"].get<std::string>();
        if (units == "meters")
        {
            x *= SCALE;
            y *= SCALE;
        }

        bool apply_offset = false;
        if (obj.contains("raw") && obj["raw"].is_boolean() && obj["raw"].get<bool>() == true)
            apply_offset = true;
        if (obj.contains("apply_offset") && obj["apply_offset"].is_boolean() && obj["apply_offset"].get<bool>() == true)
            apply_offset = true;
        if (apply_offset)
        {
            x += OFFSET_X;
            y += OFFSET_Y;
        }

        SpawnPointData sp;
        sp.x = x;
        sp.y = y;
        sp.angle = angle;
        out.push_back(sp);
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