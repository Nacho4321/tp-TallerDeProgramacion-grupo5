#include "LobbyClient.h"
#include "../common/liberror.h"
#include <iostream>

LobbyClient::LobbyClient() 
    : connected_(false), currentGameId_(0), currentPlayerId_(0) {}

LobbyClient::~LobbyClient() {
    disconnect();
}

bool LobbyClient::connect(const std::string& host, const std::string& port) {
    try {
        address_ = host;
        port_ = port;
        
        Socket s(host.c_str(), port.c_str());
        protocol_ = std::make_unique<Protocol>(std::move(s));
        handler_ = std::make_unique<GameClientHandler>(*protocol_);
        handler_->start();
        
        connected_ = true;
        currentGameId_ = 0;
        currentPlayerId_ = 0;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[LobbyClient] Error conectando: " << e.what() << std::endl;
        connected_ = false;
        return false;
    }
}


std::vector<ServerMessage::GameSummary> LobbyClient::listGames() {
    std::vector<ServerMessage::GameSummary> games;
    
    if (!connected_ || !handler_) {
        std::cerr << "[LobbyClient] No conectado" << std::endl;
        return games;
    }
    
    try {
        games = handler_->get_games_blocking();
        return games;
        
    } catch (const std::exception& e) {
        std::cerr << "[LobbyClient] Error listando partidas: " << e.what() << std::endl;
        connected_ = false;
        return games;
    }
}


bool LobbyClient::createGame(const std::string& gameName, uint32_t& outGameId, uint32_t& outPlayerId) {
    if (!connected_ || !handler_) {
        std::cerr << "[LobbyClient] No conectado, no se puede crear partida" << std::endl;
        return false;
    }
    
    try {
        bool success = handler_->create_game_blocking(outGameId, outPlayerId, gameName);
        if (success) {
            currentGameId_ = outGameId;
            currentPlayerId_ = outPlayerId;
        }
        return success;
        
    } catch (const std::exception& e) {
        std::cerr << "[LobbyClient] Error creando partida: " << e.what() << std::endl;
        return false;
    }
}


bool LobbyClient::joinGame(uint32_t gameId, uint32_t& outPlayerId) {
    if (!connected_ || !handler_) {
        std::cerr << "[LobbyClient] No conectado, no se pudo unir a la partida" << std::endl;
        return false;
    }
    
    try {
        bool success = handler_->join_game_blocking(static_cast<int32_t>(gameId), outPlayerId);
        if (success) {
            currentGameId_ = gameId;
            currentPlayerId_ = outPlayerId;
        }
        return success;
        
    } catch (const std::exception& e) {
        std::cerr << "[LobbyClient] Error uniendose a partida: " << e.what() << std::endl;
        return false;
    }
}


bool LobbyClient::startGame() {
    if (!connected_ || !handler_) {
        std::cerr << "[LobbyClient] No conectado, no se puede iniciar partida" << std::endl;
        return false;
    }
    
    if (currentGameId_ == 0) {
        std::cerr << "[LobbyClient] No hay partida para iniciar" << std::endl;
        return false;
    }
    
    try {
        handler_->send(START_GAME_STR);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[LobbyClient] Error iniciando partida: " << e.what() << std::endl;
        return false;
    }
}


void LobbyClient::leaveGame() {
    if (!connected_ || !handler_) {
        return;
    }
    
    try {
        handler_->send(LEAVE_GAME_STR);
    } catch (const std::exception& e) {
        std::cerr << "[LobbyClient] Error abandonando partida: " << e.what() << std::endl;
    }
    
    disconnect();
}


bool LobbyClient::checkGameStarted() {
    if (!connected_ || !handler_) {
        return false;
    }
    
    try {
        return handler_->wait_for_game_started();
    } catch (const std::exception& e) {
        std::cerr << "[LobbyClient] Error checkeando si inicio el jeugo: " << e.what() << std::endl;
        return false;
    }
}


bool LobbyClient::isConnected() const {
    return connected_;
}


std::string LobbyClient::getAddress() const {
    return address_;
}


std::string LobbyClient::getPort() const {
    return port_;
}


uint32_t LobbyClient::getCurrentGameId() const {
    return currentGameId_;
}


uint32_t LobbyClient::getCurrentPlayerId() const {
    return currentPlayerId_;
}


void LobbyClient::disconnect() {
    if (handler_) {
        handler_->stop();
        handler_->join();
        handler_.reset();
    }
    if (protocol_) {
        try {
            protocol_->shutdown();
        } catch (...) {}
        protocol_.reset();
    }
    connected_ = false;
    currentGameId_ = 0;
    currentPlayerId_ = 0;
}
