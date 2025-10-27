#include "protocol.h"

DecodedMessage Protocol::receiveUpPressed() {
    DecodedMessage msg;
    msg.cmd = MOVE_UP_PRESSED_STR;
    return msg;
}

DecodedMessage Protocol::receiveUpRealesed() {
    DecodedMessage msg;
    msg.cmd = MOVE_UP_RELEASED_STR;
    return msg;
}

DecodedMessage Protocol::receiveDownPressed() {
    DecodedMessage msg;
    msg.cmd = MOVE_DOWN_PRESSED_STR;
    return msg;
}

DecodedMessage Protocol::receiveDownReleased() {
    DecodedMessage msg;
    msg.cmd = MOVE_DOWN_RELEASED_STR;
    return msg;
}

DecodedMessage Protocol::receiveLeftPressed() {
    DecodedMessage msg;
    msg.cmd = MOVE_LEFT_PRESSED_STR;
    return msg;
}

DecodedMessage Protocol::receiveLeftReleased() {
    DecodedMessage msg;
    msg.cmd = MOVE_LEFT_RELEASED_STR;
    return msg;
}

DecodedMessage Protocol::receiveRightPressed() {
    DecodedMessage msg;
    msg.cmd = MOVE_RIGHT_PRESSED_STR;
    return msg;
}

DecodedMessage Protocol::receiveRightReleased() {
    DecodedMessage msg;
    msg.cmd = MOVE_RIGHT_RELEASED_STR;
    return msg;
}

DecodedMessage Protocol::receivePositionsUpdate() {
    DecodedMessage msg;
    msg.cmd = UPDATE_POSITIONS_STR;
    return msg;
}
