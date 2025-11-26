#ifndef EVENT_H
#define EVENT_H
#include <string>

struct Event
{
    int client_id = -1;        // Valor por defecto
    std::string action = "";   // Valor por defecto
    
    // Constructor sin parámetros (por defecto)
    Event() = default;
    
    // Constructor con parámetros
    Event(int client, std::string act) : client_id(client), action(act) {}
    
    virtual ~Event() = default;
};

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
    float new_X;
    float new_Y;
    MovementDirectionX direction_x;
    MovementDirectionY direction_y;
    float angle; // body orientation in radians (added so server can send actual rotation)
};
struct PlayerMovedEvent : public Event
{
    Position pos;
    explicit PlayerMovedEvent(int client_id, std::string action, Position new_position)
        : Event(client_id, action),
          pos(new_position)
    {
    }
};

#endif