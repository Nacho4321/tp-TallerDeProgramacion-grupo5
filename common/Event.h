#include <memory>

struct Event
{
    int client_id;
    uint8_t action;
    virtual ~Event() = default;
};

enum MovementDirection
{
    left = -1,
    forwmard = 0,
    right = 1,
    backwards = 2
};

struct PlayerMovedEvent : public Event
{
    float new_X;
    float new_Y;
    MovementDirection direction_x;
    MovementDirection direction_y;
    PlayerMovedEvent(int client_id, uint8_t action, float x, float y, MovementDirection dir_x, MovementDirection dir_y)
    {
        this->client_id = client_id;
        this->action = action;
        this->new_X = x;
        this->new_Y = y;
        this->direction_x = dir_x;
        this->direction_y = dir_y;
    }
};
