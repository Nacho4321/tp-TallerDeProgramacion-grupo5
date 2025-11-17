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
    MapLayout(b2World &world_map) : world(world_map) {}
};
#endif