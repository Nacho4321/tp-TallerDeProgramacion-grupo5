#ifndef MAP_LAYOUT_H
#define MAP_LAYOUT_H
#include <box2d/b2_world.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_body.h>
class MapLayout
{
private:
    b2World &world;

public:
    void create_walls();
    MapLayout(b2World &world_map) : world(world_map) {}
};
#endif