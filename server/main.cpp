#include <iostream>
#include <string>
#include <vector>

#include "server.h"
#define CANT_ARGS 2
#define PORT_ARG 1
#define FAILURE 1
#define SUCCESS 0
#define SERVER_ERROR "Error in server: "
#define SERVER_PARAMS " <port>\n"
int main(int argc, const char *argv[])
{
    try
    {
        if (argc != CANT_ARGS)
        {
            std::cerr << "Use: " << argv[0] << SERVER_PARAMS;
            return FAILURE;
        }
        Server server(argv[1]);
        server.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << SERVER_ERROR << e.what() << std::endl;
        return FAILURE;
    }
    return SUCCESS;
}