#include <memory>

struct Event
{
    int client_id;
    uint8_t action;
    virtual ~Event() = default;
};

struct PlayerMovedEvent : public Event
{
    float new_X;
    float new_Y;
    PlayerMovedEvent(int client_id, uint8_t action, float x, float y)
    {
        this->client_id = client_id;
        this->action = action;
        this->new_X = x;
        this->new_Y = y;
    }
};
