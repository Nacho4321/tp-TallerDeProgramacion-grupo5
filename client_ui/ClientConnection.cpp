#include "ClientConnection.h"
#include "../common/socket.h"
#include "../common/liberror.h"
#include <iostream>

ClientConnection::ClientConnection() = default;
ClientConnection::~ClientConnection() { disconnect(); }

void ClientConnection::setConnectionInfo(const std::string& host, const std::string& port_str) {
    // Solo guardamos la informacion
    address = host;
    port = port_str;
    connected = true;
}

void ClientConnection::disconnect() {
    protocol.reset();
    connected = false;
}

bool ClientConnection::isConnected() const {
    return connected;
}

std::string ClientConnection::getAddress() const {
    return address;
}

std::string ClientConnection::getPort() const {
    return port;
}