#include "Event.h"

Event::Event(int client, std::string act)
    : client_id(client), action(act)
{
}

PlayerMovedEvent::PlayerMovedEvent(int client_id, std::string action, Position new_position)
    : Event(client_id, action), pos(new_position)
{
}
