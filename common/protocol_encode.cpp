#include "protocol.h"
#include <iostream>


void Protocol::init_encode_handlers() {
    // Server message encode handlers
    server_encode_handlers[UPDATE_POSITIONS] = [this](ServerMessage& out) { encodeUpdatePositions(out); };
    server_encode_handlers[GAME_JOINED] = [this](ServerMessage& out) { encodeGameJoined(out); };
    server_encode_handlers[GAMES_LIST] = [this](ServerMessage& out) { encodeGamesList(out); };
    server_encode_handlers[RACE_TIMES] = [this](ServerMessage& out) { encodeRaceTimes(out); };
    server_encode_handlers[TOTAL_TIMES] = [this](ServerMessage& out) { encodeTotalTimes(out); };
    
    // Client message encode handlers 
    client_encode_handlers[CREATE_GAME] = [this](const ClientMessage& msg, uint8_t) { encodeCreateGame(msg); };
    client_encode_handlers[CHANGE_CAR] = [this](const ClientMessage& msg, uint8_t) { encodeChangeCar(msg); };
    client_encode_handlers[UPGRADE_CAR] = [this](const ClientMessage& msg, uint8_t) { encodeUpgrade(msg); };
    client_encode_handlers[CHEAT_CMD] = [this](const ClientMessage& msg, uint8_t) { encodeCheat(msg); };
}


std::vector<std::uint8_t> Protocol::encodeClientMessage(const ClientMessage &msg)
{
    buffer.clear();
    const std::string &cmd = msg.cmd;

    // Buscar opcode en el mapa
    auto cmd_it = cmd_to_opcode.find(cmd);
    if (cmd_it == cmd_to_opcode.end()) {
        std::cerr << "[Protocol(Client)] Unknown command: " << cmd << std::endl;
        return buffer;
    }
    
    uint8_t opcode = cmd_it->second;

    // Header común: opcode + player_id + game_id
    buffer.push_back(opcode);
    insertUint32(static_cast<uint32_t>(msg.player_id));
    insertUint32(static_cast<uint32_t>(msg.game_id));
    
    // Payload específico usando dispatch
    auto it = client_encode_handlers.find(opcode);
    if (it != client_encode_handlers.end()) {
        it->second(msg, opcode);
    }
    
    return buffer;
}


void Protocol::encodeCreateGame(const ClientMessage& msg) {
    insertString(msg.game_name);
    buffer.push_back(msg.map_id);
}

void Protocol::encodeChangeCar(const ClientMessage& msg) {
    insertString(msg.car_type);
}

void Protocol::encodeUpgrade(const ClientMessage& msg) {
    buffer.push_back(static_cast<uint8_t>(msg.upgrade_type));
}

void Protocol::encodeCheat(const ClientMessage& msg) {
    buffer.push_back(static_cast<uint8_t>(msg.cheat_type));
}



std::vector<std::uint8_t> Protocol::encodeServerMessage(ServerMessage &out)
{
    buffer.clear();
    
    auto it = server_encode_handlers.find(out.opcode);
    if (it != server_encode_handlers.end()) {
        it->second(out);
    } else {
        encodeDefaultOpcode(out);
    }
    
    return buffer;
}

// Helpers de encode, ServerMessage

void Protocol::encodeUpdatePositions(ServerMessage& out) {
    buffer.push_back(UPDATE_POSITIONS);
    buffer.push_back(static_cast<std::uint8_t>(out.positions.size()));

    for (auto &pos_update : out.positions) {
        insertInt(pos_update.player_id);
        insertPosition(pos_update.new_pos);

        // Checkpoints
        uint8_t next_count = static_cast<uint8_t>(pos_update.next_checkpoints.size());
        buffer.push_back(next_count);

        for (const auto &cp : pos_update.next_checkpoints) {
            insertPosition(cp);
        }

        // Car type
        insertString(pos_update.car_type);

        // HP y flags
        insertFloat(pos_update.hp);
        buffer.push_back(pos_update.collision_flag ? 1 : 0);

        // Upgrades
        buffer.push_back(pos_update.upgrade_speed);
        buffer.push_back(pos_update.upgrade_acceleration);
        buffer.push_back(pos_update.upgrade_handling);
        buffer.push_back(pos_update.upgrade_durability);
        
        // Frenazo bool
        buffer.push_back(pos_update.is_stopping ? 1 : 0);
    }
}

void Protocol::encodeGameJoined(ServerMessage& out) {
    buffer.push_back(GAME_JOINED);
    insertUint32(out.game_id);
    insertUint32(out.player_id);
    buffer.push_back(out.success ? 1 : 0);
    buffer.push_back(out.map_id);
}

void Protocol::encodeGamesList(ServerMessage& out) {
    buffer.push_back(GAMES_LIST);
    insertUint32(static_cast<uint32_t>(out.games.size()));
    for (auto &g : out.games) {
        insertUint32(g.game_id);
        insertUint32(g.player_count);
        buffer.push_back(g.map_id);
        insertString(g.name);
    }
}

void Protocol::encodeRaceTimes(ServerMessage& out) {
    buffer.push_back(RACE_TIMES);
    insertUint32(static_cast<uint32_t>(out.race_times.size()));
    for (const auto &rt : out.race_times) {
        insertUint32(rt.player_id);
        insertUint32(rt.time_ms);
        buffer.push_back(rt.disqualified ? 1 : 0);
        buffer.push_back(rt.round_index);
    }
}

void Protocol::encodeTotalTimes(ServerMessage& out) {
    buffer.push_back(TOTAL_TIMES);
    insertUint32(static_cast<uint32_t>(out.total_times.size()));
    for (const auto &tt : out.total_times) {
        insertUint32(tt.player_id);
        insertUint32(tt.total_ms);
    }
}

void Protocol::encodeDefaultOpcode(ServerMessage& out) {
    buffer.push_back(out.opcode);
}

// ==================== LEGACY FUNCTIONS ====================

std::vector<std::uint8_t> Protocol::encodeOpcode(std::uint8_t opcode)
{
    buffer.clear();
    buffer.push_back(opcode);
    return buffer;
}

std::vector<std::uint8_t> Protocol::encodeGameJoinedResponse(const GameJoinedResponse &response)
{
    buffer.clear();
    buffer.push_back(GAME_JOINED);
    insertUint32(response.game_id);
    insertUint32(response.player_id);
    buffer.push_back(response.success ? 1 : 0);
    return buffer;
}
