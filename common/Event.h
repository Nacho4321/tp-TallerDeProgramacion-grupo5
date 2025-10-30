#include <memory>

struct Event
{
    int client_id;
    uint8_t action;
    explicit Event(int client, uint8_t act) : client_id(client), action(act) {}
    virtual ~Event() = default;
};

enum MovementDirection
{
    left = -1,
    forward = 0,
    right = 1,
    backwards = 2
};

struct PlayerMovedEvent : public Event
{
    float new_X;
    float new_Y;
    MovementDirection direction_x;
    MovementDirection direction_y;
    PlayerMovedEvent(int client_id, uint8_t action, float x, float y,
                     MovementDirection dir_x, MovementDirection dir_y)
        : Event(client_id, action),
          new_X(x), new_Y(y),
          direction_x(dir_x), direction_y(dir_y)
    {
    }
};
