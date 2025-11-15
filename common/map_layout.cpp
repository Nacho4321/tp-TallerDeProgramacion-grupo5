#include "map_layout.h"

#define MAP_WIDTH (4640.0f / 32.0f)
#define MAP_HEIGHT (4672.0f / 32.0f)
#define WALL_THICKNESS 0.1f
void MapLayout::create_walls()
{
    b2BodyDef bd;
    bd.position.Set(0, 0);
    b2Body *worldBounds = world.CreateBody(&bd);

    b2PolygonShape shape_upper;
    shape_upper.SetAsBox(MAP_WIDTH / 2.0f, WALL_THICKNESS,
                         b2Vec2(MAP_WIDTH / 2.0f, WALL_THICKNESS),
                         0.0f);
    worldBounds->CreateFixture(&shape_upper, 0.0f);

    b2PolygonShape shape_lower;
    shape_lower.SetAsBox(MAP_WIDTH / 2.0f, WALL_THICKNESS,
                         b2Vec2(MAP_WIDTH / 2.0f, MAP_HEIGHT - WALL_THICKNESS * 0.5f),
                         0.0f);
    worldBounds->CreateFixture(&shape_lower, 0.0f);

    b2PolygonShape shape_left;
    shape_left.SetAsBox(WALL_THICKNESS, MAP_HEIGHT / 2.0f,
                        b2Vec2(WALL_THICKNESS, MAP_HEIGHT / 2.0f),
                        0.0f);
    worldBounds->CreateFixture(&shape_left, 0.0f);

    b2PolygonShape shape_right;
    shape_right.SetAsBox(WALL_THICKNESS, MAP_HEIGHT / 2.0f,
                         b2Vec2(MAP_WIDTH - WALL_THICKNESS, MAP_HEIGHT / 2.0f),
                         0.0f);
    worldBounds->CreateFixture(&shape_right, 0.0f);
}
