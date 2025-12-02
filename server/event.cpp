#include "event.h"

Event::Event(int client, std::string act)
    : client_id(client), action(act)
{
}

