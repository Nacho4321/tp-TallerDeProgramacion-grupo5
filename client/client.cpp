#include "client.h"
#include <iostream>
#include <string>
void Client::start()
{
    bool connected = true;
    std::string input;
    while (connected)
    {
        std::getline(std::cin, input);
    }
}