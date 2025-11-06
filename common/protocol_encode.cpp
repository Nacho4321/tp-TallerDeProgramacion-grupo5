#include "protocol.h"

std::vector<std::uint8_t> Protocol::encodeClientMessage(const ClientMessage& msg) {
    buffer.clear();
    uint8_t opcode = 0;
    // Mapear el comando a opcode
    const std::string& cmd = msg.cmd;
    if (cmd == MOVE_UP_PRESSED_STR) opcode = MOVE_UP_PRESSED;
    else if (cmd == MOVE_UP_RELEASED_STR) opcode = MOVE_UP_RELEASED;
    else if (cmd == MOVE_DOWN_PRESSED_STR) opcode = MOVE_DOWN_PRESSED;
    else if (cmd == MOVE_DOWN_RELEASED_STR) opcode = MOVE_DOWN_RELEASED;
    else if (cmd == MOVE_LEFT_PRESSED_STR) opcode = MOVE_LEFT_PRESSED;
    else if (cmd == MOVE_LEFT_RELEASED_STR) opcode = MOVE_LEFT_RELEASED;
    else if (cmd == MOVE_RIGHT_PRESSED_STR) opcode = MOVE_RIGHT_PRESSED;
    else if (cmd == MOVE_RIGHT_RELEASED_STR) opcode = MOVE_RIGHT_RELEASED;
    else if (cmd == CREATE_GAME_STR) opcode = CREATE_GAME;
    else if (cmd.rfind(JOIN_GAME_STR, 0) == 0) opcode = JOIN_GAME;
    else opcode = 0; // desconocido

    buffer.push_back(opcode);
    // Siempre incluimos player_id y game_id (8 bytes)
    insertUint32(static_cast<uint32_t>(msg.player_id));
    insertUint32(static_cast<uint32_t>(msg.game_id));
    // No hay payload adicional excepto para JOIN_GAME que ya reutiliza game_id
    return buffer;
}

std::vector<std::uint8_t> Protocol::encodeOpcode(std::uint8_t opcode) {
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

std::vector<std::uint8_t> Protocol::encodeGameJoinedResponse(const GameJoinedResponse& response) {
    buffer.clear();
    buffer.push_back(GAME_JOINED);
    insertUint32(response.game_id);
    insertUint32(response.player_id);
    buffer.push_back(response.success ? 1 : 0);
    return buffer;
}
