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
    void create_polygon_layout(const std::vector<b2Vec2> &vertices, uint16_t category);

public:
    void create_map_layout(const std::string &jsonPath);
    // Extrae checkpoints de un archivo JSON y los coloca en 'out' como b2Vec2 en metros
    // El JSON debe tener el formato correcto: { "checkpoints": [ {"id": 1, "x": 960, "y": 540}, ... ] }
    void extract_checkpoints(const std::string &jsonPath, std::vector<b2Vec2> &out);

    // Extrae waypoints NPC de un archivo JSON similar a checkpoints pero con conectividad
    // Formato: { "waypoints": [ {"id": 0, "x": 100, "y": 100, "connections": [1, 3]}, ... ] }
    struct WaypointData
    {
        b2Vec2 position;
        std::vector<int> connections;
    };
    void extract_npc_waypoints(const std::string &jsonPath, std::vector<WaypointData> &out);

    // Extrae posiciones de autos estacionados de un archivo JSON
    // Formato: { "parked_cars": [ {"x": 100, "y": 100, "horizontal": true}, ... ] }
    struct ParkedCarData
    {
        b2Vec2 position;
        bool horizontal; // true = orientado horizontalmente, false = vertical
    };
    void extract_parked_cars(const std::string &jsonPath, std::vector<ParkedCarData> &out);

    MapLayout(b2World &world_map) : world(world_map) {}
};
#endif