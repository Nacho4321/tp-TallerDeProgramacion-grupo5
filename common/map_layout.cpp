#include "map_layout.h"

#define MAP_WIDTH (4640.0f / 32.0f)
#define MAP_HEIGHT (4672.0f / 32.0f)
#define SCALE 32.0f
#define WALL_THICKNESS 0.1f
#define OFFSET_X -10.0f
#define OFFSET_Y -10.0f

void MapLayout::create_walls()
{
    b2BodyDef bd;
    bd.position.Set(0, 0);
    b2Body *worldBounds = world.CreateBody(&bd);

    b2PolygonShape shape_upper;
    shape_upper.SetAsBox(MAP_WIDTH / 2.0f, WALL_THICKNESS,
                         b2Vec2(MAP_WIDTH / 2.0f + OFFSET_X / SCALE, WALL_THICKNESS + OFFSET_Y / SCALE),
                         0.0f);
    worldBounds->CreateFixture(&shape_upper, 0.0f);

    b2PolygonShape shape_lower;
    shape_lower.SetAsBox(MAP_WIDTH / 2.0f, WALL_THICKNESS,
                         b2Vec2(MAP_WIDTH / 2.0f + OFFSET_X / SCALE, MAP_HEIGHT - WALL_THICKNESS * 0.5f + OFFSET_Y / SCALE),
                         0.0f);
    worldBounds->CreateFixture(&shape_lower, 0.0f);

    b2PolygonShape shape_left;
    shape_left.SetAsBox(WALL_THICKNESS, MAP_HEIGHT / 2.0f,
                        b2Vec2(WALL_THICKNESS + OFFSET_X / SCALE, MAP_HEIGHT / 2.0f + OFFSET_Y / SCALE),
                        0.0f);
    worldBounds->CreateFixture(&shape_left, 0.0f);

    b2PolygonShape shape_right;
    shape_right.SetAsBox(WALL_THICKNESS, MAP_HEIGHT / 2.0f,
                         b2Vec2(MAP_WIDTH - WALL_THICKNESS + OFFSET_X / SCALE, MAP_HEIGHT / 2.0f + OFFSET_Y / SCALE),
                         0.0f);
    worldBounds->CreateFixture(&shape_right, 0.0f);
}

void MapLayout::create_map_layout()
{
    create_walls();
    create_square_layout(116.88f, 126.16f, 212.38f, 372.34f);
}

void MapLayout::create_square_layout(const float &x, const float &y, const float &width, const float &height)
{
    float adjX = x + OFFSET_X;
    float adjY = y + OFFSET_Y;

    float centerX = (adjX + width / 2.0f) / SCALE;
    float centerY = (adjY + height / 2.0f) / SCALE;
    float halfWidth = width / 2.0f / SCALE;
    float halfHeight = height / 2.0f / SCALE;

    b2BodyDef bd;
    bd.position.Set(centerX, centerY);
    b2Body *block = world.CreateBody(&bd);

    b2PolygonShape shape;
    shape.SetAsBox(halfWidth, halfHeight);
    block->CreateFixture(&shape, 0.0f);
}
