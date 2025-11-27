#ifndef POSITION_H
#define POSITION_H

enum MovementDirectionX
{
    left = -1,
    not_horizontal = 0,
    right = 1,
};

enum MovementDirectionY
{
    up = -1,
    not_vertical = 0,
    down = 1,
};

struct Position
{
    bool on_bridge;
    float new_X;
    float new_Y;
    MovementDirectionX direction_x;
    MovementDirectionY direction_y;
    float angle; // body orientation in radians (added so server can send actual rotation)
};

#endif
