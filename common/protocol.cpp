
#include "protocol.h"
#include <iostream>
#include <utility>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>

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
    receive_handlers[UPGRADE_CAR] = [this]() {
        auto msg = receiveUpgradeCar();
        std::cout << "[Protocol(Server)] Decodificado UPGRADE_CAR player_id=" << msg.player_id
                  << " game_id=" << msg.game_id << " upgrade_type=" << static_cast<int>(msg.upgrade_type) << std::endl;
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
    cmd_to_opcode[UPGRADE_CAR_STR] = UPGRADE_CAR;
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
    if (outOpcode == GAME_STARTED) {
        outServer.opcode = GAME_STARTED;
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
    std::cout << "[Protocol] receiveAnyServerPacket: opcode desconocido " << int(outOpcode) << std::endl;
     return false;
}

void Protocol::sendMessage(ServerMessage& out) {
    switch (out.opcode) {

    case CREATE_GAME:
        std::cout << "Sending CREATE_GAME\n";
        break;

    case JOIN_GAME:
        std::cout << "Sending JOIN_GAME\n";
        break;

    case GAME_JOINED:
        std::cout << "Sending GAME_JOINED: game_id=" << out.game_id
                  << " player_id=" << out.player_id
                  << " success=" << out.success << "\n";
        break;

    case GET_GAMES:
        std::cout << "Sending GET_GAMES\n";
        break;

    case GAMES_LIST:
        std::cout << "Sending GAMES_LIST (" << out.games.size() << " games)\n";
        break;

    case START_GAME:
        std::cout << "Sending START_GAME\n";
        break;

    case GAME_STARTED:
        std::cout << "Sending GAME_STARTED\n";
        break;

    case STARTING_COUNTDOWN:
        std::cout << "Sending STARTING_COUNTDOWN\n";
        break;

    case RACE_TIMES:
        std::cout << "Sending RACE_TIMES (" << out.race_times.size() << " results)\n";
        break;

    case TOTAL_TIMES:
        std::cout << "Sending TOTAL_TIMES (" << out.total_times.size() << " totals)\n";
        break;
    case UPDATE_POSITIONS:
        break;
    default:
        std::cout << "Sending UNKNOWN OPCODE: 0x"
                  << std::hex << (int)out.opcode << std::dec << "\n";
        break;
    }

    // Encode and send
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
