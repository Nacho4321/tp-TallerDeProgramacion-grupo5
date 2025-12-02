#include "protocol.h"

void Protocol::readClientIds(ClientMessage &msg)
{
    readBuffer.resize(sizeof(uint32_t) * 2);
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
    {
        msg.player_id = -1;
        msg.game_id = -1;
        return;
    }
    size_t idx = 0;
    msg.player_id = static_cast<int32_t>(exportUint32(readBuffer, idx));
    msg.game_id = static_cast<int32_t>(exportUint32(readBuffer, idx));
}

ClientMessage Protocol::receiveUpPressed()
{
    ClientMessage msg;
    msg.cmd = MOVE_UP_PRESSED_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveUpRealesed()
{
    ClientMessage msg;
    msg.cmd = MOVE_UP_RELEASED_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveDownPressed()
{
    ClientMessage msg;
    msg.cmd = MOVE_DOWN_PRESSED_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveDownReleased()
{
    ClientMessage msg;
    msg.cmd = MOVE_DOWN_RELEASED_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveLeftPressed()
{
    ClientMessage msg;
    msg.cmd = MOVE_LEFT_PRESSED_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveLeftReleased()
{
    ClientMessage msg;
    msg.cmd = MOVE_LEFT_RELEASED_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveRightPressed()
{
    ClientMessage msg;
    msg.cmd = MOVE_RIGHT_PRESSED_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveRightReleased()
{
    ClientMessage msg;
    msg.cmd = MOVE_RIGHT_RELEASED_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveCreateGame()
{
    ClientMessage msg;
    msg.cmd = CREATE_GAME_STR;
    readClientIds(msg);
    // Leer nombre
    readString(msg.game_name);
    // Leer map_id
    uint8_t map_id_buf = 0;
    if (skt.recvall(&map_id_buf, 1) > 0)
    {
        msg.map_id = map_id_buf;
    }
    return msg;
}

ClientMessage Protocol::receiveJoinGame()
{
    ClientMessage msg;
    msg.cmd = JOIN_GAME_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveGetGames()
{
    ClientMessage msg;
    msg.cmd = GET_GAMES_STR;
    readClientIds(msg);
    return msg;
}

ClientMessage Protocol::receiveStartGame()
{
    ClientMessage msg;
    msg.cmd = START_GAME_STR;
    readClientIds(msg);
    return msg;
}

// Helper para decodificar STARTING_COUNTDOWN (sin payload)
ServerMessage Protocol::receiveStartingCountdown() {
    ServerMessage out;
    out.opcode = STARTING_COUNTDOWN;
    // No hay payload, solo el opcode
    return out;
}

ClientMessage Protocol::receiveChangeCar()
{
    ClientMessage msg;
    msg.cmd = CHANGE_CAR_STR;
    readClientIds(msg);
    // Leer car_type
    if (readString(msg.car_type) && !msg.car_type.empty()) {
        // Reconstruir cmd completo para que llegue como evento legible: "change_car <tipo>"
        msg.cmd = std::string(CHANGE_CAR_STR) + " " + msg.car_type;
    }
    return msg;
}

ClientMessage Protocol::receiveUpgradeCar()
{
    ClientMessage msg;
    msg.cmd = "upgrade_car";
    readClientIds(msg);
    // Leer upgrade_type 
    uint8_t upgrade_byte;
    if (skt.recvall(&upgrade_byte, sizeof(upgrade_byte)) <= 0)
        return msg;
    msg.upgrade_type = static_cast<CarUpgrade>(upgrade_byte);
    msg.cmd = std::string("upgrade_car") + " " + std::to_string(upgrade_byte);
    return msg;
}

ClientMessage Protocol::receiveCheat()
{
    ClientMessage msg;
    msg.cmd = "cheat";
    readClientIds(msg);
    // Leer cheat_type
    uint8_t cheat_byte;
    if (skt.recvall(&cheat_byte, sizeof(cheat_byte)) <= 0)
        return msg;
    msg.cheat_type = static_cast<CheatType>(cheat_byte);
    // Construir cmd con el tipo de cheat para el dispatcher
    switch (msg.cheat_type) {
        case CheatType::GOD_MODE:
            msg.cmd = CHEAT_GOD_MODE_STR;
            break;
        case CheatType::DIE:
            msg.cmd = CHEAT_DIE_STR;
            break;
        case CheatType::SKIP_LAP:
            msg.cmd = CHEAT_SKIP_LAP_STR;
            break;
        case CheatType::FULL_UPGRADE:
            msg.cmd = CHEAT_FULL_UPGRADE_STR;
            break;
    }
    return msg;
}

// Helpers de receive para receivePositionsUpdate

bool Protocol::readPosition(Position& pos) {
    // on_bridge (1) + direction_x (1) + direction_y (1) + X (4) + Y (4) + angle (4) = 15 bytes
    readBuffer.resize(sizeof(uint8_t) + 2 * sizeof(int8_t) + 3 * sizeof(float));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
        return false;

    size_t idx = 0;
    pos.on_bridge = (readBuffer[idx++] != 0);
    pos.direction_x = static_cast<MovementDirectionX>(static_cast<int8_t>(readBuffer[idx++]));
    pos.direction_y = static_cast<MovementDirectionY>(static_cast<int8_t>(readBuffer[idx++]));
    pos.new_X = exportFloat(readBuffer, idx);
    pos.new_Y = exportFloat(readBuffer, idx);
    pos.angle = exportFloat(readBuffer, idx);
    return true;
}

bool Protocol::readString(std::string& str) {
    readBuffer.resize(sizeof(uint16_t));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
        return false;
    
    size_t idx = 0;
    uint16_t len = exportUint16(readBuffer, idx);
    
    if (len > 0) {
        std::vector<uint8_t> strBuf(len);
        if (skt.recvall(strBuf.data(), strBuf.size()) <= 0)
            return false;
        str.assign(reinterpret_cast<char*>(strBuf.data()), strBuf.size());
    }
    return true;
}

bool Protocol::readPlayerPositionUpdate(PlayerPositionUpdate& update) {
    // player_id (4 bytes)
    readBuffer.resize(sizeof(int32_t));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
        return false;
    size_t idx = 0;
    update.player_id = exportInt(readBuffer, idx);

    // Posición principal
    if (!readPosition(update.new_pos))
        return false;

    // Checkpoints
    uint8_t next_count = 0;
    if (skt.recvall(&next_count, sizeof(next_count)) <= 0)
        return false;

    for (uint8_t k = 0; k < next_count; ++k) {
        Position cp{};
        if (!readPosition(cp))
            return false;
        update.next_checkpoints.push_back(cp);
    }

    // Car type
    if (!readString(update.car_type))
        return false;

    // HP
    readBuffer.resize(sizeof(float));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
        return false;
    idx = 0;
    update.hp = exportFloat(readBuffer, idx);

    // Collision flag
    uint8_t collision_byte;
    if (skt.recvall(&collision_byte, sizeof(collision_byte)) <= 0)
        return false;
    update.collision_flag = (collision_byte != 0);

    // Upgrades (4 bytes)
    uint8_t upgrade_bytes[4];
    if (skt.recvall(upgrade_bytes, sizeof(upgrade_bytes)) <= 0)
        return false;
    update.upgrade_speed = upgrade_bytes[0];
    update.upgrade_acceleration = upgrade_bytes[1];
    update.upgrade_handling = upgrade_bytes[2];
    update.upgrade_durability = upgrade_bytes[3];

    // Is stopping
    uint8_t stopping_byte;
    if (skt.recvall(&stopping_byte, sizeof(stopping_byte)) <= 0)
        return false;
    update.is_stopping = (stopping_byte != 0);

    return true;
}


ServerMessage Protocol::receivePositionsUpdate()
{
    ServerMessage msg;
    msg.opcode = UPDATE_POSITIONS;

    uint8_t count;
    if (skt.recvall(&count, sizeof(count)) <= 0)
        return msg;

    for (int i = 0; i < count; i++) {
        PlayerPositionUpdate update;
        if (!readPlayerPositionUpdate(update))
            return msg;
        msg.positions.push_back(update);
    }

    return msg;
}

GameJoinedResponse Protocol::receiveGameJoinedResponse()
{
    GameJoinedResponse resp;

    // Leer game_id (uint32) + player_id (uint32) + success (uint8) + map_id (uint8)
    readBuffer.resize(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
    {
        resp.success = false;
        resp.game_id = 0;
        resp.player_id = 0;
        resp.map_id = 0;
        return resp;
    }

    size_t idx = 0;
    resp.game_id = exportUint32(readBuffer, idx);
    resp.player_id = exportUint32(readBuffer, idx);
    uint8_t success_byte;
    std::memcpy(&success_byte, readBuffer.data() + idx, sizeof(uint8_t));
    idx += sizeof(uint8_t);
    resp.success = (success_byte != 0);
    resp.map_id = readBuffer[idx];

    return resp;
}

ServerMessage Protocol::receiveGamesList()
{
    ServerMessage msg;
    msg.opcode = GAMES_LIST;
    // Leer cantidad
    readBuffer.resize(sizeof(uint32_t));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
        return msg;
    size_t idx = 0;
    uint32_t count = exportUint32(readBuffer, idx);
    for (uint32_t i = 0; i < count; ++i)
    {
        // game_id (4) + player_count (4) + map_id (1) + nameLen (2) = 11 bytes
        readBuffer.resize(sizeof(uint32_t) * 2 + sizeof(uint8_t) + sizeof(uint16_t));
        if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
            return msg;
        size_t j = 0;
        ServerMessage::GameSummary summary{};
        summary.game_id = exportUint32(readBuffer, j);
        summary.player_count = exportUint32(readBuffer, j);
        summary.map_id = readBuffer[j++];
        uint16_t nameLen = exportUint16(readBuffer, j);
        if (nameLen > 0)
        {
            std::vector<uint8_t> nb(nameLen);
            if (skt.recvall(nb.data(), nb.size()) <= 0)
                return msg;
            summary.name.assign(reinterpret_cast<char *>(nb.data()), nb.size());
        }
        msg.games.push_back(std::move(summary));
    }
    return msg;
}

ServerMessage Protocol::receiveRaceTimes()
{
    ServerMessage out;
    out.opcode = RACE_TIMES;
    readBuffer.resize(sizeof(uint32_t));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0) return out;
    size_t idx = 0;
    uint32_t count = exportUint32(readBuffer, idx);
    out.race_times.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        readBuffer.resize(sizeof(uint32_t) * 2);
        if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0) return out;
        size_t j = 0;
        uint32_t pid = exportUint32(readBuffer, j);
        uint32_t tms = exportUint32(readBuffer, j);
        uint8_t dq = 0;
        if (skt.recvall(&dq, sizeof(dq)) <= 0) return out;
        uint8_t round = 0;
        if (skt.recvall(&round, sizeof(round)) <= 0) return out;
        out.race_times.push_back({pid, tms, dq != 0, round});
    }
    return out;
}

ServerMessage Protocol::receiveTotalTimes()
{
    ServerMessage out;
    out.opcode = TOTAL_TIMES;
    readBuffer.resize(sizeof(uint32_t));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0) return out;
    size_t idx = 0;
    uint32_t count = exportUint32(readBuffer, idx);
    out.total_times.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        readBuffer.resize(sizeof(uint32_t) * 2);
        if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0) return out;
        size_t j = 0;
        uint32_t pid = exportUint32(readBuffer, j);
        uint32_t total = exportUint32(readBuffer, j);
        out.total_times.push_back({pid, total});
    }
    return out;
}
