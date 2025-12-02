#include "game_connection.h"
#include <iostream>

GameConnection::GameConnection()
    : protocol_(nullptr)
    , handler_(nullptr)
    , address_("")
    , port_("")
    , gameId_(0)
    , playerId_(0)
    , mapId_(0)
    , connected_(false)
    , started_(false)
{}

GameConnection::~GameConnection() {
    shutdown();
}

GameConnection::GameConnection(GameConnection&& other) noexcept
    : protocol_(std::move(other.protocol_))
    , handler_(std::move(other.handler_))
    , address_(std::move(other.address_))
    , port_(std::move(other.port_))
    , gameId_(other.gameId_)
    , playerId_(other.playerId_)
    , mapId_(other.mapId_)
    , connected_(other.connected_)
    , started_(other.started_)
{
    other.gameId_ = 0;
    other.playerId_ = 0;
    other.mapId_ = 0;
    other.connected_ = false;
    other.started_ = false;
}

GameConnection& GameConnection::operator=(GameConnection&& other) noexcept {
    if (this != &other) {
        shutdown();
        
        protocol_ = std::move(other.protocol_);
        handler_ = std::move(other.handler_);
        address_ = std::move(other.address_);
        port_ = std::move(other.port_);
        gameId_ = other.gameId_;
        playerId_ = other.playerId_;
        mapId_ = other.mapId_;
        connected_ = other.connected_;
        started_ = other.started_;

        other.gameId_ = 0;
        other.playerId_ = 0;
        other.mapId_ = 0;
        other.connected_ = false;
        other.started_ = false;
    }
    return *this;
}

bool GameConnection::connect(const std::string& host, const std::string& port) {
    try {
        address_ = host;
        port_ = port;
        
        Socket socket(host.c_str(), port.c_str());
        protocol_ = std::make_unique<Protocol>(std::move(socket));
        handler_ = std::make_unique<GameClientHandler>(*protocol_);
        
        connected_ = true;
        started_ = false;
        gameId_ = 0;
        playerId_ = 0;
        mapId_ = 0;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[GameConnection] Error conectando: " << e.what() << std::endl;
        connected_ = false;
        return false;
    }
}

void GameConnection::start() {
    if (handler_ && !started_) {
        handler_->start();
        started_ = true;
    }
}

void GameConnection::stop() {
    if (handler_ && started_) {
        handler_->stop();
    }
}

void GameConnection::join() {
    if (handler_ && started_) {
        handler_->join();
        started_ = false;
    }
}

void GameConnection::shutdown() {
    if (started_) {
        stop();
        join();
    }
    
    if (protocol_) {
        try {
            protocol_->shutdown();
        } catch (...) {}
        protocol_.reset();
    }
    
    handler_.reset();
    connected_ = false;
    gameId_ = 0;
    playerId_ = 0;
}

void GameConnection::send(const std::string& msg) {
    if (handler_ && started_) {
        handler_->send(msg);
    }
}

bool GameConnection::tryReceive(ServerMessage& out) {
    if (handler_ && started_) {
        return handler_->try_receive(out);
    }
    return false;
}

bool GameConnection::createGame(const std::string& gameName, uint32_t& outGameId, uint32_t& outPlayerId, uint8_t mapId) {
    if (!connected_ || !handler_ || !started_) {
        std::cerr << "[GameConnection] No conectado o no iniciado" << std::endl;
        return false;
    }
    
    try {
        uint8_t outMapId = 0;
        bool success = handler_->create_game_blocking(outGameId, outPlayerId, outMapId, gameName, mapId);
        if (success) {
            gameId_ = outGameId;
            playerId_ = outPlayerId;
            mapId_ = outMapId;
        }
        return success;
        
    } catch (const std::exception& e) {
        std::cerr << "[GameConnection] Error creando partida: " << e.what() << std::endl;
        return false;
    }
}

bool GameConnection::joinGame(uint32_t gameId, uint32_t& outPlayerId) {
    if (!connected_ || !handler_ || !started_) {
        std::cerr << "[GameConnection] No conectado o no iniciado" << std::endl;
        return false;
    }
    
    try {
        uint8_t outMapId = 0;
        bool success = handler_->join_game_blocking(static_cast<int32_t>(gameId), outPlayerId, outMapId);
        if (success) {
            gameId_ = gameId;
            playerId_ = outPlayerId;
            mapId_ = outMapId;
        }
        return success;
        
    } catch (const std::exception& e) {
        std::cerr << "[GameConnection] Error uniéndose a partida: " << e.what() << std::endl;
        return false;
    }
}

bool GameConnection::startGame() {
    if (!connected_ || !handler_ || !started_) {
        std::cerr << "[GameConnection] No conectado" << std::endl;
        return false;
    }
    
    if (gameId_ == 0) {
        std::cerr << "[GameConnection] No hay partida activa" << std::endl;
        return false;
    }
    
    try {
        handler_->send(START_GAME_STR);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[GameConnection] Error iniciando partida: " << e.what() << std::endl;
        return false;
    }
}

bool GameConnection::checkGameStarted() {
    if (!connected_ || !handler_ || !started_) {
        return false;
    }
    
    try {
        return handler_->wait_for_game_started();
    } catch (const std::exception& e) {
        std::cerr << "[GameConnection] Error verificando inicio de juego: " << e.what() << std::endl;
        return false;
    }
}

std::vector<ServerMessage::GameSummary> GameConnection::listGames() {
    std::vector<ServerMessage::GameSummary> games;
    
    if (!connected_ || !handler_ || !started_) {
        std::cerr << "[GameConnection] No conectado o no iniciado" << std::endl;
        return games;
    }
    
    try {
        return handler_->get_games_blocking();
    } catch (const std::exception& e) {
        std::cerr << "[GameConnection] Error listando juegos: " << e.what() << std::endl;
        return games;
    }
}

bool GameConnection::selectCar(const std::string& carType) {
    if (!connected_ || !handler_ || !started_) {
        std::cerr << "[GameConnection] No conectado, no se puede seleccionar auto" << std::endl;
        return false;
    }
    
    try {
        std::string cmd = std::string(CHANGE_CAR_STR) + " " + carType;
        handler_->send(cmd);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool GameConnection::isConnected() const {
    return connected_;
}

bool GameConnection::isStarted() const {
    return started_;
}

std::string GameConnection::getAddress() const {
    return address_;
}

std::string GameConnection::getPort() const {
    return port_;
}

uint32_t GameConnection::getGameId() const {
    return gameId_;
}

uint32_t GameConnection::getPlayerId() const {
    return playerId_;
}

uint8_t GameConnection::getMapId() const {
    return mapId_;
}
