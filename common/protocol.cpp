#include "protocol.h"

#include <iostream>
#include <utility>

#include <netinet/in.h>

Protocol::Protocol(Socket&& socket) noexcept: skt(std::move(socket)), buffer() {}

ClientMessage Protocol::receiveClientMessage() {
    uint8_t opcode;
    if (skt.recvall(&opcode, sizeof(opcode)) <= 0)
        return {};

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

void Protocol::sendMessage(ServerMessage& out) {
    auto msg = encodeServerMessage(out);
    skt.sendall(msg.data(), msg.size());
}

void Protocol::sendMessage(ClientMessage& out) {
    auto msg = encodeCommand(out.cmd);
    skt.sendall(msg.data(), msg.size());
}

void Protocol::shutdown() {
    try {
        skt.shutdown(2);
    } catch (...) {}
}
