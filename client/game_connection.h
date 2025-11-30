#ifndef GAME_CONNECTION_H
#define GAME_CONNECTION_H

#include <string>
#include <memory>
#include <vector>
#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/messages.h"
#include "game_client_handler.h"

/** 
 * Aca esta encapsulada la logica de conexion del cliente, se transfiere de QT a SDL para mantener una sola conexion.
 */

class GameConnection {
private:
    std::unique_ptr<Protocol> protocol_;
    std::unique_ptr<GameClientHandler> handler_;
    
    std::string address_;
    std::string port_;
    
    uint32_t gameId_;
    uint32_t playerId_;
    
    bool connected_;
    bool started_;

public:
    GameConnection();
    ~GameConnection();
    
    GameConnection(const GameConnection&) = delete;
    GameConnection& operator=(const GameConnection&) = delete;

    GameConnection(GameConnection&& other) noexcept;
    GameConnection& operator=(GameConnection&& other) noexcept;
    
    bool connect(const std::string& host, const std::string& port);
    void start();
    void stop();
    void join();
    void shutdown();
    
    void send(const std::string& msg);
    bool tryReceive(ServerMessage& out);
    
    bool createGame(const std::string& gameName, uint32_t& outGameId, uint32_t& outPlayerId);
    bool joinGame(uint32_t gameId, uint32_t& outPlayerId);
    bool startGame();
    bool checkGameStarted();
    std::vector<ServerMessage::GameSummary> listGames();

    bool selectCar(const std::string& carType);
    
    bool isConnected() const;
    bool isStarted() const;
    
    std::string getAddress() const;
    std::string getPort() const;
    uint32_t getGameId() const;
    uint32_t getPlayerId() const;

    GameClientHandler* getHandler() { return handler_.get(); }
};

#endif 
