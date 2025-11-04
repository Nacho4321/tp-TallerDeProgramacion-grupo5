#include "protocol.h"

ClientMessage Protocol::receiveUpPressed() {
    ClientMessage msg;
    msg.cmd = MOVE_UP_PRESSED_STR;
    return msg;
}

ClientMessage Protocol::receiveUpRealesed() {
    ClientMessage msg;
    msg.cmd = MOVE_UP_RELEASED_STR;
    return msg;
}

ClientMessage Protocol::receiveDownPressed() {
    ClientMessage msg;
    msg.cmd = MOVE_DOWN_PRESSED_STR;
    return msg;
}

ClientMessage Protocol::receiveDownReleased() {
    ClientMessage msg;
    msg.cmd = MOVE_DOWN_RELEASED_STR;
    return msg;
}

ClientMessage Protocol::receiveLeftPressed() {
    ClientMessage msg;
    msg.cmd = MOVE_LEFT_PRESSED_STR;
    return msg;
}

ClientMessage Protocol::receiveLeftReleased() {
    ClientMessage msg;
    msg.cmd = MOVE_LEFT_RELEASED_STR;
    return msg;
}

ClientMessage Protocol::receiveRightPressed() {
    ClientMessage msg;
    msg.cmd = MOVE_RIGHT_PRESSED_STR;
    return msg;
}

ClientMessage Protocol::receiveRightReleased() {
    ClientMessage msg;
    msg.cmd = MOVE_RIGHT_RELEASED_STR;
    return msg;
}

ServerMessage Protocol::receivePositionsUpdate() {
    ServerMessage msg;
    
    // Leer cantidad de posiciones
    uint8_t count;
    if (skt.recvall(&count, sizeof(count)) <= 0)
        return msg;
    
    // Leer cada posición
    for (int i = 0; i < count; i++) {
        PlayerPositionUpdate update;
        
    // Recibir datos en el buffer temporal
    // Cada PlayerPositionUpdate serializa 5 uint32/int32-sized values:
    // player_id (int32), new_X (float->uint32), new_Y (float->uint32),
    // direction_x (int32), direction_y (int32) => 5 * 4 = 20 bytes
    readBuffer.resize(5 * sizeof(uint32_t));
        if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
            return msg;
            
        size_t idx = 0;
        update.player_id = exportInt(readBuffer, idx);
        update.new_pos.new_X = exportFloat(readBuffer, idx);
        update.new_pos.new_Y = exportFloat(readBuffer, idx);
        update.new_pos.direction_x = static_cast<MovementDirectionX>(exportInt(readBuffer, idx));
        update.new_pos.direction_y = static_cast<MovementDirectionY>(exportInt(readBuffer, idx));
        
        msg.positions.push_back(update);
    }
    
    return msg;
}
