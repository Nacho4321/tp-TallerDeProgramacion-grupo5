#include "protocol.h"

std::vector<std::uint8_t> Protocol::encodeCommand(const std::string& cmd) {

    buffer.clear();
    if (cmd == MOVE_UP_PRESSED_STR) {
        buffer = encodeClientMessage(MOVE_UP_PRESSED);
    } else if (cmd == MOVE_UP_RELEASED_STR) {
        buffer = encodeClientMessage(MOVE_UP_RELEASED);
    } else if (cmd == MOVE_DOWN_PRESSED_STR) {
        buffer = encodeClientMessage(MOVE_DOWN_PRESSED);
    } else if (cmd == MOVE_DOWN_RELEASED_STR) {
        buffer = encodeClientMessage(MOVE_DOWN_RELEASED);
    } else if (cmd == MOVE_LEFT_PRESSED_STR) {
        buffer = encodeClientMessage(MOVE_LEFT_PRESSED);
    } else if (cmd == MOVE_LEFT_RELEASED_STR) {
        buffer = encodeClientMessage(MOVE_LEFT_RELEASED);
    } else if (cmd == MOVE_RIGHT_PRESSED_STR) {
        buffer = encodeClientMessage(MOVE_RIGHT_PRESSED);
    } else if (cmd == MOVE_RIGHT_RELEASED_STR) {
        buffer = encodeClientMessage(MOVE_RIGHT_RELEASED);
    } 
    return buffer;
}

std::vector<std::uint8_t> Protocol::encodeClientMessage(std::uint8_t opcode) {
    buffer.clear();
    buffer.push_back(opcode);
    return buffer;
}

std::vector<std::uint8_t> Protocol::encodeServerMessage(ServerMessage& out) {
    buffer.clear();
    buffer.push_back(UPDATE_POSITIONS);
    buffer.push_back(static_cast<std::uint8_t>(out.positions.size()));
    for (auto & pos_update : out.positions) {
        insertInt(pos_update.player_id);
        insertFloat(pos_update.new_pos.new_X);
        insertFloat(pos_update.new_pos.new_Y);
        insertInt(static_cast<int>(pos_update.new_pos.direction_x));
        insertInt(static_cast<int>(pos_update.new_pos.direction_y));
    }    
    return buffer;
}



