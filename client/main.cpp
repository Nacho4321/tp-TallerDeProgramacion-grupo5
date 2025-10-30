#include <iostream>
#include <exception>
#include "client.h"

#define ADDRESS_ARG 1
#define PORT_ARG 2
#define CANT_ARGS 3
#define CLIENT_ERROR "Error in client: "
#define CLIENT_PARAMS " <address> <port>\n"

int main(int argc, const char *argv[])
{
    try
    {
        if (argc != CANT_ARGS)
        {
            std::cerr << "Use: " << argv[0] << CLIENT_PARAMS;
            return 1;
        }

        Client client(argv[ADDRESS_ARG], argv[PORT_ARG]);
        client.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << CLIENT_ERROR << e.what() << std::endl;
        return 1;
    }
    return 0;
}