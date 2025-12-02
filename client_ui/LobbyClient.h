#ifndef LOBBY_CLIENT_H
#define LOBBY_CLIENT_H

#include <string>
#include <vector>
#include <memory>
#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/messages.h"
#include "../client/game_connection.h"

class LobbyClient {
private:
    std::unique_ptr<GameConnection> connection_;
    std::string host;
    std::string port;

public:
    LobbyClient();
    ~LobbyClient();

    bool connect(const std::string& host, const std::string& port);
    
    std::vector<ServerMessage::GameSummary> listGames();
    
    bool createGame(const std::string& gameName, uint32_t& outGameId, uint32_t& outPlayerId, uint8_t mapId = 0);
    bool joinGame(uint32_t gameId, uint32_t& outPlayerId);
    bool startGame();
    void leaveGame();
    bool checkGameStarted();
    bool selectCar(const std::string& carType);
    
    bool isConnected() const;
    std::string getAddress() const;
    std::string getPort() const;
    uint32_t getCurrentGameId() const;
    uint32_t getCurrentPlayerId() const;
    
    void disconnect();
    
    // este metodo sirve para pasarle la conexion a SDL
    std::unique_ptr<GameConnection> extractConnection();
};

#endif
