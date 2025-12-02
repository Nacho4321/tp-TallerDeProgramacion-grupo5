#ifndef EVENT_H
#define EVENT_H
#include <string>
#include "../common/position.h"

struct Event
{
    int client_id = -1;      // Valor por defecto
    std::string action = ""; // Valor por defecto

    // Constructor sin parámetros (por defecto)
    Event() = default;

    // Constructor con parámetros
    Event(int client, std::string act);

    virtual ~Event() = default;
};
#endif