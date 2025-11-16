#ifndef MAP_LAYOUT_H
#define MAP_LAYOUT_H
#include <box2d/b2_world.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_body.h>
class MapLayout
{
private:
    b2World &world;

    void create_walls();
    void create_square_layout(const float &x, const float &y, const float &width, const float &height);

public:
    void create_map_layout();
    MapLayout(b2World &world_map) : world(world_map) {}
};
#endif