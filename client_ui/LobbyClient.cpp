#include "LobbyClient.h"
#include "../common/liberror.h"
#include <iostream>

LobbyClient::LobbyClient() 
    : connection_(nullptr) {}

LobbyClient::~LobbyClient() {
    disconnect();
}

bool LobbyClient::connect(const std::string& host, const std::string& port) {
    try {
        connection_ = std::make_unique<GameConnection>();
        
        if (!connection_->connect(host, port)) {
            connection_.reset();
            return false;
        }
        
        connection_->start();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[LobbyClient] Error conectando: " << e.what() << std::endl;
        connection_.reset();
        return false;
    }
}


std::vector<ServerMessage::GameSummary> LobbyClient::listGames() {
    if (!connection_ || !connection_->isConnected()) {
        std::cerr << "[LobbyClient] No conectado" << std::endl;
        return {};
    }
    
    return connection_->listGames();
}


bool LobbyClient::createGame(const std::string& gameName, uint32_t& outGameId, uint32_t& outPlayerId, uint8_t mapId) {
    if (!connection_ || !connection_->isConnected()) {
        std::cerr << "[LobbyClient] No conectado, no se puede crear partida" << std::endl;
        return false;
    }
    
    return connection_->createGame(gameName, outGameId, outPlayerId, mapId);
}


bool LobbyClient::joinGame(uint32_t gameId, uint32_t& outPlayerId) {
    if (!connection_ || !connection_->isConnected()) {
        std::cerr << "[LobbyClient] No conectado, no se pudo unir a la partida" << std::endl;
        return false;
    }
    
    return connection_->joinGame(gameId, outPlayerId);
}


bool LobbyClient::startGame() {
    if (!connection_ || !connection_->isConnected()) {
        std::cerr << "[LobbyClient] No conectado, no se puede iniciar partida" << std::endl;
        return false;
    }
    
    return connection_->startGame();
}


void LobbyClient::leaveGame() {
    disconnect();
}


bool LobbyClient::checkGameStarted() {
    if (!connection_ || !connection_->isConnected()) {
        return false;
    }
    
    return connection_->checkGameStarted();
}


bool LobbyClient::selectCar(const std::string& carType) {
    if (!connection_ || !connection_->isConnected()) {
        std::cerr << "[LobbyClient] No conectado, no se pudo seleccionar auto" << std::endl;
        return false;
    }
    
    return connection_->selectCar(carType);
}


bool LobbyClient::isConnected() const {
    return connection_ && connection_->isConnected();
}


std::string LobbyClient::getAddress() const {
    return connection_ ? connection_->getAddress(): "";
}


std::string LobbyClient::getPort() const {
    return connection_ ? connection_->getPort() : "";
}


uint32_t LobbyClient::getCurrentGameId() const {
    return connection_ ? connection_->getGameId() : 0;
}


uint32_t LobbyClient::getCurrentPlayerId() const {
    return connection_ ? connection_->getPlayerId() : 0;
}


void LobbyClient::disconnect() {
    if (connection_) {
        connection_->shutdown();
        connection_.reset();
    }
}


std::unique_ptr<GameConnection> LobbyClient::extractConnection() {
    return std::move(connection_);
}
