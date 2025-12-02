
#include "protocol.h"
#include <iostream>
#include <utility>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>

Protocol::Protocol(Socket&& socket) noexcept: skt(std::move(socket)), buffer() {
    init_handlers();
    init_cmd_map();
    init_encode_handlers();
    init_server_receive_handlers();
}

void Protocol::init_handlers() {
    // Inicializar mapa de handlers para receiveClientMessage con lambdas
    receive_handlers[MOVE_UP_PRESSED] = [this]() { return receiveUpPressed(); };
    receive_handlers[MOVE_UP_RELEASED] = [this]() { return receiveUpRealesed(); };
    receive_handlers[MOVE_DOWN_PRESSED] = [this]() { return receiveDownPressed(); };
    receive_handlers[MOVE_DOWN_RELEASED] = [this]() { return receiveDownReleased(); };
    receive_handlers[MOVE_LEFT_PRESSED] = [this]() { return receiveLeftPressed(); };
    receive_handlers[MOVE_LEFT_RELEASED] = [this]() { return receiveLeftReleased(); };
    receive_handlers[MOVE_RIGHT_PRESSED] = [this]() { return receiveRightPressed(); };
    receive_handlers[MOVE_RIGHT_RELEASED] = [this]() { return receiveRightReleased(); };

    receive_handlers[CREATE_GAME] = [this]() { return receiveCreateGame();};
    receive_handlers[JOIN_GAME] = [this]() { return receiveJoinGame(); };
    receive_handlers[GET_GAMES] = [this]() { return receiveGetGames(); };
    receive_handlers[START_GAME] = [this]() { return receiveStartGame(); };

    receive_handlers[CHANGE_CAR] = [this]() { return receiveChangeCar(); };
    receive_handlers[UPGRADE_CAR] = [this]() { return receiveUpgradeCar(); };

    receive_handlers[CHEAT_CMD] = [this]() { return receiveCheat(); };
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
    cmd_to_opcode[UPGRADE_CAR_STR] = UPGRADE_CAR;
    // Upgrade car 
    cmd_to_opcode[std::string(UPGRADE_CAR_STR) + " 0"] = UPGRADE_CAR;
    cmd_to_opcode[std::string(UPGRADE_CAR_STR) + " 1"] = UPGRADE_CAR;
    cmd_to_opcode[std::string(UPGRADE_CAR_STR) + " 2"] = UPGRADE_CAR;
    cmd_to_opcode[std::string(UPGRADE_CAR_STR) + " 3"] = UPGRADE_CAR;
    // Cheats
    cmd_to_opcode[CHEAT_GOD_MODE_STR] = CHEAT_CMD;
    cmd_to_opcode[CHEAT_DIE_STR] = CHEAT_CMD;
    cmd_to_opcode[CHEAT_SKIP_LAP_STR] = CHEAT_CMD;
    cmd_to_opcode[CHEAT_FULL_UPGRADE_STR] = CHEAT_CMD;
}

void Protocol::init_server_receive_handlers() {
    server_receive_handlers[UPDATE_POSITIONS] = [this](ServerMessage& out, GameJoinedResponse&) {
        out = receivePositionsUpdate();
    };
    server_receive_handlers[GAME_JOINED] = [this](ServerMessage&, GameJoinedResponse& joined) {
        joined = receiveGameJoinedResponse();
    };
    server_receive_handlers[GAMES_LIST] = [this](ServerMessage& out, GameJoinedResponse&) {
        out = receiveGamesList();
    };
    server_receive_handlers[GAME_STARTED] = [](ServerMessage& out, GameJoinedResponse&) {
        out.opcode = GAME_STARTED;
    };
    server_receive_handlers[STARTING_COUNTDOWN] = [this](ServerMessage& out, GameJoinedResponse&) {
        out = receiveStartingCountdown();
    };
    server_receive_handlers[RACE_TIMES] = [this](ServerMessage& out, GameJoinedResponse&) {
        out = receiveRaceTimes();
    };
    server_receive_handlers[TOTAL_TIMES] = [this](ServerMessage& out, GameJoinedResponse&) {
        out = receiveTotalTimes();
    };
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
    ssize_t recv_result = skt.recvall(&outOpcode, sizeof(outOpcode));
    if (recv_result <= 0) {
        if (recv_result == 0) {
            std::cout << "[Protocol] receiveAnyServerPacket: conexión cerrada por el servidor (recv=0)" << std::endl;
        } else {
            std::cout << "[Protocol] receiveAnyServerPacket: error en recv, errno=" << errno 
                      << " (" << strerror(errno) << ")" << std::endl;
        }
        return false;
    }
    
    auto it = server_receive_handlers.find(outOpcode);
    if (it != server_receive_handlers.end()) {
        it->second(outServer, outJoined);
        return true;
    }
    
    std::cerr << "[Protocol] receiveAnyServerPacket: opcode desconocido " << int(outOpcode) << std::endl;
    return false;
}

void Protocol::sendMessage(ServerMessage& out) {
    auto msg = encodeServerMessage(out);
    skt.sendall(msg.data(), msg.size());
}

void Protocol::sendMessage(ClientMessage& out) {
    auto msg = encodeClientMessage(out);
    skt.sendall(msg.data(), msg.size());
}


void Protocol::shutdown() {
    try {
        skt.shutdown(2);
    } catch (...) {}
}
