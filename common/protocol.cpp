#include "protocol.h"

#include <iostream>
#include <utility>

#include <netinet/in.h>

Protocol::Protocol(Socket&& socket) noexcept: skt(std::move(socket)), buffer() {}

ClientMessage Protocol::receiveClientMessage() {
    uint8_t opcode;
    if (skt.recvall(&opcode, sizeof(opcode)) <= 0)
        return {};
    // DEBUG: log opcode crudo recibido
    std::cout << "[Protocol(Server)] Raw opcode recibido=" << int(opcode) << std::endl;

    switch (opcode) {
        case MOVE_UP_PRESSED:
            return receiveUpPressed();
        case MOVE_UP_RELEASED:
            return receiveUpRealesed();
        case MOVE_DOWN_PRESSED:
            return receiveDownPressed();
        case MOVE_DOWN_RELEASED:
            return receiveDownReleased();
        case MOVE_LEFT_PRESSED:
            return receiveLeftPressed();
        case MOVE_LEFT_RELEASED:
            return receiveLeftReleased();
        case MOVE_RIGHT_PRESSED:
            return receiveRightPressed();
        case MOVE_RIGHT_RELEASED:
            return receiveRightReleased();
        case CREATE_GAME:
            {
                auto msg = receiveCreateGame();
                std::cout << "[Protocol(Server)] Decodificado CREATE_GAME player_id=" << msg.player_id
                          << " game_id=" << msg.game_id << std::endl;
                return msg;
            }
        case JOIN_GAME:
            {
                auto msg = receiveJoinGame();
                std::cout << "[Protocol(Server)] Decodificado JOIN_GAME player_id=" << msg.player_id
                          << " game_id=" << msg.game_id << std::endl;
                return msg;
            }
        case GET_GAMES:
            {
                auto msg = receiveGetGames();
                std::cout << "[Protocol(Server)] Decodificado GET_GAMES player_id=" << msg.player_id
                          << " game_id=" << msg.game_id << std::endl;
                return msg;
            }
        case CHANGE_CAR:
            {
                auto msg = receiveChangeCar();
                std::cout << "[Protocol(Server)] Decodificado CHANGE_CAR player_id=" << msg.player_id
                          << " game_id=" << msg.game_id << " car_type=" << msg.car_type << std::endl;
                return msg;
            }
        default:
            return {};  // desconocido
    }
}

ServerMessage Protocol::receiveServerMessage() {
    uint8_t opcode;
    if (skt.recvall(&opcode, sizeof(opcode)) <= 0)
        return {};

    if (opcode == UPDATE_POSITIONS) {
        return receivePositionsUpdate();
    } else {
        return {};  // desconocido
    }
}

bool Protocol::receiveAnyServerPacket(ServerMessage& outServer,
                                      GameJoinedResponse& outJoined,
                                      uint8_t& outOpcode) {
    if (skt.recvall(&outOpcode, sizeof(outOpcode)) <= 0) return false;
    if (outOpcode == UPDATE_POSITIONS) {
        outServer = receivePositionsUpdate();
        return true;
    }
    if (outOpcode == GAME_JOINED) {
        outJoined = receiveGameJoinedResponse();
        return true;
    }
    if (outOpcode == GAMES_LIST) {
        outServer = receiveGamesList();
        return true;
    }
    return false;
}

void Protocol::sendMessage(ServerMessage& out) {
    auto msg = encodeServerMessage(out);
    skt.sendall(msg.data(), msg.size());
}

void Protocol::sendMessage(ClientMessage& out) {
    auto msg = encodeClientMessage(out);
    std::cout << "[Protocol(Client)] sendMessage cmd='" << out.cmd << "' bytes=" << msg.size()
              << " opcode_first=" << (msg.empty()? -1 : int(msg[0]))
              << " player_id=" << out.player_id << " game_id=" << out.game_id << std::endl;
    skt.sendall(msg.data(), msg.size());
}

void Protocol::sendMessage(const GameJoinedResponse& response) {
    auto msg = encodeGameJoinedResponse(response);
    skt.sendall(msg.data(), msg.size());
}

// Cliente recibe respuesta del servidor
GameJoinedResponse Protocol::receiveGameJoined() {
    uint8_t opcode;
    if (skt.recvall(&opcode, sizeof(opcode)) <= 0) {
        return {0, 0, false};
    }
    
    if (opcode == GAME_JOINED) {
        return receiveGameJoinedResponse();
    }
    
    return {0, 0, false};
}

void Protocol::shutdown() {
    try {
        skt.shutdown(2);
    } catch (...) {}
}
