#ifndef LOBBY_CLIENT_H
#define LOBBY_CLIENT_H

#include <string>
#include <vector>
#include <memory>
#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/messages.h"
#include "../client/game_client_handler.h"

class LobbyClient {
private:
    std::unique_ptr<Protocol> protocol_;
    std::unique_ptr<GameClientHandler> handler_;
    std::string address_;
    std::string port_;
    bool connected_;

public:
    LobbyClient();
    ~LobbyClient();

    bool connect(const std::string& host, const std::string& port);
    
    std::vector<ServerMessage::GameSummary> listGames();
    
    // Getters
    bool isConnected() const { return connected_; }
    std::string getAddress() const { return address_; }
    std::string getPort() const { return port_; }
    
    void disconnect();
};

#endif
