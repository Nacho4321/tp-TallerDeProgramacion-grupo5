
#include "protocol.h"
#include <iostream>
#include <utility>
#include <netinet/in.h>


Protocol::Protocol(Socket&& socket) noexcept: skt(std::move(socket)), buffer() {
    init_handlers();
    init_cmd_map();
}

void Protocol::init_handlers() {
    // Inicializar mapa de handlers para receiveClientMessage
    receive_handlers[MOVE_UP_PRESSED] = [this]() { return receiveUpPressed(); };
    receive_handlers[MOVE_UP_RELEASED] = [this]() { return receiveUpRealesed(); };
    receive_handlers[MOVE_DOWN_PRESSED] = [this]() { return receiveDownPressed(); };
    receive_handlers[MOVE_DOWN_RELEASED] = [this]() { return receiveDownReleased(); };
    receive_handlers[MOVE_LEFT_PRESSED] = [this]() { return receiveLeftPressed(); };
    receive_handlers[MOVE_LEFT_RELEASED] = [this]() { return receiveLeftReleased(); };
    receive_handlers[MOVE_RIGHT_PRESSED] = [this]() { return receiveRightPressed(); };
    receive_handlers[MOVE_RIGHT_RELEASED] = [this]() { return receiveRightReleased(); };
    
    receive_handlers[CREATE_GAME] = [this]() {
        auto msg = receiveCreateGame();
        std::cout << "[Protocol(Server)] Decodificado CREATE_GAME player_id=" << msg.player_id
                  << " game_id=" << msg.game_id << std::endl;
        return msg;
    };
    
    receive_handlers[JOIN_GAME] = [this]() {
        auto msg = receiveJoinGame();
        std::cout << "[Protocol(Server)] Decodificado JOIN_GAME player_id=" << msg.player_id
                  << " game_id=" << msg.game_id << std::endl;
        return msg;
    };
    
    receive_handlers[GET_GAMES] = [this]() {
        auto msg = receiveGetGames();
        std::cout << "[Protocol(Server)] Decodificado GET_GAMES player_id=" << msg.player_id
                  << " game_id=" << msg.game_id << std::endl;
        return msg;
    };
    
    receive_handlers[START_GAME] = [this]() {
        auto msg = receiveStartGame();
        std::cout << "[Protocol(Server)] Decodificado START_GAME player_id=" << msg.player_id
                  << " game_id=" << msg.game_id << std::endl;
        return msg;
    };
    
    receive_handlers[CHANGE_CAR] = [this]() {
        auto msg = receiveChangeCar();
        std::cout << "[Protocol(Server)] Decodificado CHANGE_CAR player_id=" << msg.player_id
                  << " game_id=" << msg.game_id << " car_type=" << msg.car_type << std::endl;
        return msg;
    };
}

void Protocol::init_cmd_map() {
    // Inicializar mapa de comandos string a opcode para encodeClientMessage
    cmd_to_opcode[MOVE_UP_PRESSED_STR] = MOVE_UP_PRESSED;
    cmd_to_opcode[MOVE_UP_RELEASED_STR] = MOVE_UP_RELEASED;
    cmd_to_opcode[MOVE_DOWN_PRESSED_STR] = MOVE_DOWN_PRESSED;
    cmd_to_opcode[MOVE_DOWN_RELEASED_STR] = MOVE_DOWN_RELEASED;
    cmd_to_opcode[MOVE_LEFT_PRESSED_STR] = MOVE_LEFT_PRESSED;
    cmd_to_opcode[MOVE_LEFT_RELEASED_STR] = MOVE_LEFT_RELEASED;
    cmd_to_opcode[MOVE_RIGHT_PRESSED_STR] = MOVE_RIGHT_PRESSED;
    cmd_to_opcode[MOVE_RIGHT_RELEASED_STR] = MOVE_RIGHT_RELEASED;
    cmd_to_opcode[CREATE_GAME_STR] = CREATE_GAME;
    cmd_to_opcode[JOIN_GAME_STR] = JOIN_GAME;
    cmd_to_opcode[GET_GAMES_STR] = GET_GAMES;
    cmd_to_opcode[START_GAME_STR] = START_GAME;
    cmd_to_opcode[CHANGE_CAR_STR] = CHANGE_CAR;
}

ClientMessage Protocol::receiveClientMessage() {
    uint8_t opcode;
    if (skt.recvall(&opcode, sizeof(opcode)) <= 0)
        return {};

    // Buscar handler en el mapa
    auto it = receive_handlers.find(opcode);
    if (it != receive_handlers.end()) {
        return it->second();  // Invocar handler
    }
    
    return {};  // Opcode desconocido
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
    if (outOpcode == STARTING_COUNTDOWN) {
        outServer = receiveStartingCountdown();
        return true;
    }
    if (outOpcode == RACE_TIMES) {
        outServer = receiveRaceTimes();
        return true;
    }
    if (outOpcode == TOTAL_TIMES) {
        outServer = receiveTotalTimes();
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


void Protocol::shutdown() {
    try {
        skt.shutdown(2);
    } catch (...) {}
}
