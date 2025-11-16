#include "map_layout.h"
#include <nlohmann/json.hpp>
#include <fstream>

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

        if (layer["name"] != "Collisions")
            continue;

        for (auto &obj : layer["objects"])
        {
            if (!obj.contains("polygon"))
                continue;

            float baseX = obj["x"].get<float>();
            float baseY = obj["y"].get<float>();

            std::vector<b2Vec2> verts;

            for (auto &pt : obj["polygon"])
            {
                float px = baseX + pt["x"].get<float>() + OFFSET_X;
                float py = baseY + pt["y"].get<float>() + OFFSET_Y;

                verts.emplace_back(px / SCALE, py / SCALE);
            }

            create_polygon_layout(verts);
        }
    }
}

void MapLayout::create_polygon_layout(const std::vector<b2Vec2> &vertices)
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

    body->CreateFixture(&fd);
}
