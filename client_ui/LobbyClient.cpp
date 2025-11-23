#include "LobbyClient.h"
#include "../common/liberror.h"
#include <iostream>

LobbyClient::LobbyClient() : connected_(false) {}

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


bool LobbyClient::isConnected() const {
    return connected_;
}


std::string LobbyClient::getAddress() const {
    return address_;
}


std::string LobbyClient::getPort() const {
    return port_;
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
}
