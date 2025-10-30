#include "protocol.h"

std::vector<std::uint8_t> Protocol::encodeCommand(const std::string& cmd) {

    buffer.clear();
    if (cmd == MOVE_UP_PRESSED_STR) {
        buffer.push_back(MOVE_UP_PRESSED);
    } else if (cmd == MOVE_UP_RELEASED_STR) {
        buffer.push_back(MOVE_UP_RELEASED);
    } else if (cmd == MOVE_DOWN_PRESSED_STR) {
        buffer.push_back(MOVE_DOWN_PRESSED);
    } else if (cmd == MOVE_DOWN_RELEASED_STR) {
        buffer.push_back(MOVE_DOWN_RELEASED);
    } else if (cmd == MOVE_LEFT_PRESSED_STR) {
        buffer.push_back(MOVE_LEFT_PRESSED);
    } else if (cmd == MOVE_LEFT_RELEASED_STR) {
        buffer.push_back(MOVE_LEFT_RELEASED);
    } else if (cmd == MOVE_RIGHT_PRESSED_STR) {
        buffer.push_back(MOVE_RIGHT_PRESSED);
    } else if (cmd == MOVE_RIGHT_RELEASED_STR) {
        buffer.push_back(MOVE_RIGHT_RELEASED);
    } else if (cmd == UPDATE_POSITIONS_STR) {
        buffer.push_back(UPDATE_POSITIONS);
    }
    return buffer;
}

