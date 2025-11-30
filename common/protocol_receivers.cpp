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
    readBuffer.resize(sizeof(uint16_t));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
        return msg; // nombre vacío si falla
    size_t idx = 0;
    uint16_t len = exportUint16(readBuffer, idx);
    if (len > 0)
    {
        std::vector<uint8_t> nameBuf(len);
        if (skt.recvall(nameBuf.data(), nameBuf.size()) > 0)
        {
            msg.game_name.assign(reinterpret_cast<char *>(nameBuf.data()), nameBuf.size());
        }
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
    // Leer car_type length + string
    readBuffer.resize(sizeof(uint16_t));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
        return msg;
    size_t idx = 0;
    uint16_t len = exportUint16(readBuffer, idx);
    if (len > 0)
    {
        std::vector<uint8_t> typeBuf(len);
        if (skt.recvall(typeBuf.data(), typeBuf.size()) > 0)
        {
            msg.car_type.assign(reinterpret_cast<char *>(typeBuf.data()), typeBuf.size());
            // Reconstruir cmd completo para que llegue como evento legible: "change_car <tipo>"
            msg.cmd = std::string(CHANGE_CAR_STR) + " " + msg.car_type;
        }
    }
    return msg;
}

ServerMessage Protocol::receivePositionsUpdate()
{
    ServerMessage msg;
    msg.opcode = UPDATE_POSITIONS;

    // Leer cantidad de posiciones
    uint8_t count;
    if (skt.recvall(&count, sizeof(count)) <= 0)
        return msg;

    // Leer cada posición
    for (int i = 0; i < count; i++)
    {
        PlayerPositionUpdate update;

        // player_id (int32) + on_bridge (uint8) + direction_x (int8) + direction_y (int8) + new_X (float) + new_Y (float) + angle (float)
        readBuffer.resize(sizeof(int32_t) + sizeof(uint8_t) + 2 * sizeof(int8_t) + 3 * sizeof(float));
        if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
            return msg;

        size_t idx = 0;
        update.player_id = exportInt(readBuffer, idx);

        uint8_t on_bridge_byte;
        std::memcpy(&on_bridge_byte, readBuffer.data() + idx, sizeof(uint8_t));
        idx += sizeof(uint8_t);
        update.new_pos.on_bridge = (on_bridge_byte != 0);

        int8_t dir_x, dir_y;
        std::memcpy(&dir_x, readBuffer.data() + idx, sizeof(int8_t));
        idx += sizeof(int8_t);
        std::memcpy(&dir_y, readBuffer.data() + idx, sizeof(int8_t));
        idx += sizeof(int8_t);
        update.new_pos.direction_x = static_cast<MovementDirectionX>(dir_x);
        update.new_pos.direction_y = static_cast<MovementDirectionY>(dir_y);

        update.new_pos.new_X = exportFloat(readBuffer, idx);
        update.new_pos.new_Y = exportFloat(readBuffer, idx);
        update.new_pos.angle = exportFloat(readBuffer, idx);

        // Leer cantidad de checkpoints
        uint8_t next_count = 0;
        if (skt.recvall(&next_count, sizeof(next_count)) <= 0)
            return msg;

        for (uint8_t k = 0; k < next_count; ++k)
        {
            readBuffer.resize(sizeof(uint8_t) + 2 * sizeof(int8_t) + 3 * sizeof(float));
            if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
                return msg;

            size_t idx_cp = 0;
            Position cp{};

            uint8_t cp_on_bridge_byte;
            std::memcpy(&cp_on_bridge_byte, readBuffer.data() + idx_cp, sizeof(uint8_t));
            idx_cp += sizeof(uint8_t);
            cp.on_bridge = (cp_on_bridge_byte != 0);

            int8_t cp_dir_x, cp_dir_y;
            std::memcpy(&cp_dir_x, readBuffer.data() + idx_cp, sizeof(int8_t));
            idx_cp += sizeof(int8_t);
            std::memcpy(&cp_dir_y, readBuffer.data() + idx_cp, sizeof(int8_t));
            idx_cp += sizeof(int8_t);
            cp.direction_x = static_cast<MovementDirectionX>(cp_dir_x);
            cp.direction_y = static_cast<MovementDirectionY>(cp_dir_y);

            cp.new_X = exportFloat(readBuffer, idx_cp);
            cp.new_Y = exportFloat(readBuffer, idx_cp);
            cp.angle = exportFloat(readBuffer, idx_cp);

            update.next_checkpoints.push_back(cp);
        }

        // Leer car_type
        readBuffer.resize(sizeof(uint16_t));
        if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
            return msg;

        size_t idx_len = 0;
        uint16_t carLen = exportUint16(readBuffer, idx_len);

        if (carLen > 0)
        {
            std::vector<uint8_t> carBuf(carLen);
            if (skt.recvall(carBuf.data(), carBuf.size()) <= 0)
                return msg;
            update.car_type.assign(reinterpret_cast<char *>(carBuf.data()), carBuf.size());
        }

        // Leer HP
        readBuffer.resize(sizeof(float));
        if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
            return msg;
        size_t idx_hp = 0;
        update.hp = exportFloat(readBuffer, idx_hp);

        // Leer collision flag
        uint8_t collision_byte;
        if (skt.recvall(&collision_byte, sizeof(collision_byte)) <= 0)
            return msg;
        update.collision_flag = (collision_byte != 0);

        msg.positions.push_back(update);
    }

    return msg;
}

GameJoinedResponse Protocol::receiveGameJoinedResponse()
{
    GameJoinedResponse resp;

    // Leer game_id (uint32) + player_id (uint32) + success (uint8)
    readBuffer.resize(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t));
    if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
    {
        resp.success = false;
        resp.game_id = 0;
        resp.player_id = 0;
        return resp;
    }

    size_t idx = 0;
    resp.game_id = exportUint32(readBuffer, idx);
    resp.player_id = exportUint32(readBuffer, idx);
    uint8_t success_byte;
    std::memcpy(&success_byte, readBuffer.data() + idx, sizeof(uint8_t));
    resp.success = (success_byte != 0);

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
        readBuffer.resize(sizeof(uint32_t) * 2 + sizeof(uint16_t));
        if (skt.recvall(readBuffer.data(), readBuffer.size()) <= 0)
            return msg;
        size_t j = 0;
        ServerMessage::GameSummary summary{};
        summary.game_id = exportUint32(readBuffer, j);
        summary.player_count = exportUint32(readBuffer, j);
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
