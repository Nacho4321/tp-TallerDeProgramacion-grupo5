#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <string>
#include <memory>
#include "../common/protocol.h"

class ClientConnection {
private:
    std::unique_ptr<Protocol> protocol;
    std::string address;
    std::string port;
    bool connected{false};

public:
    ClientConnection();
    ~ClientConnection();

    void setConnectionInfo(const std::string& host, const std::string& port);
    
    bool isConnected() const;
    std::string getAddress() const;
    std::string getPort() const;
    void disconnect();
};

#endif
