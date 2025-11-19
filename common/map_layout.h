#ifndef MAP_LAYOUT_H
#define MAP_LAYOUT_H
#include <box2d/box2d.h>
#include <vector>
#include <string>
class MapLayout
{
private:
    b2World &world;

    void create_walls();
    void create_polygon_layout(const std::vector<b2Vec2> &vertices);

public:
    void create_map_layout(const std::string &jsonPath);
    // Extrae checkpoints de un archivo JSON y los coloca en 'out' como b2Vec2 en metros
    // El JSON debe tener el formato correcto: { "checkpoints": [ {"id": 1, "x": 960, "y": 540}, ... ] }
    void extract_checkpoints(const std::string &jsonPath, std::vector<b2Vec2> &out);
    MapLayout(b2World &world_map) : world(world_map) {}
};
#endif