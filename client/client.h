#ifndef CLIENT_H
#define CLIENT_H
#include "../common/protocol.h"
class Client
{
private:
    Protocol protocol;
    Socket ini_protocol(const char *address, const char *port)
    {
        Socket sock(address, port);
        return sock;
    }

public:
    explicit Client(const char *address, const char *port) : protocol(ini_protocol(address, port)) {}
    void start();
};
#endif