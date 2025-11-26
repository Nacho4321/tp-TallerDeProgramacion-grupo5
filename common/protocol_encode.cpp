#include "protocol.h"
#include <iostream>

std::vector<std::uint8_t> Protocol::encodeClientMessage(const ClientMessage& msg) {
    buffer.clear();
    uint8_t opcode = 0;
    
    // Buscar opcode en el mapa
    const std::string& cmd = msg.cmd;
    
    // Casos especiales que requieren prefix matching
    if (cmd.rfind(JOIN_GAME_STR, 0) == 0) {
        opcode = JOIN_GAME;
        std::cout << "[Protocol(Client)] Encoding JOIN_GAME with ids p=" << msg.player_id << " g=" << msg.game_id << std::endl;
    } else if (cmd.rfind(CHANGE_CAR_STR, 0) == 0) {
        opcode = CHANGE_CAR;
    } else {
        // Búsqueda directa en el mapa
        auto it = cmd_to_opcode.find(cmd);
        if (it != cmd_to_opcode.end()) {
            opcode = it->second;
            
            // Logs especiales para ciertos comandos
            if (opcode == CREATE_GAME) {
                std::cout << "[Protocol(Client)] Encoding CREATE_GAME name='" << msg.game_name << "' p=" << msg.player_id << " g=" << msg.game_id << std::endl;
            } else if (opcode == GET_GAMES) {
                std::cout << "[Protocol(Client)] Encoding GET_GAMES" << std::endl;
            } else if (opcode == START_GAME) {
                std::cout << "[Protocol(Client)] Encoding START_GAME" << std::endl;
            }
        }
    }

    buffer.push_back(opcode);
    // Siempre incluimos player_id y game_id (8 bytes)
    insertUint32(static_cast<uint32_t>(msg.player_id));
    insertUint32(static_cast<uint32_t>(msg.game_id));
    // Payload adicional para CREATE_GAME: nombre de la partida
    if (opcode == CREATE_GAME) {
        uint16_t len = static_cast<uint16_t>(msg.game_name.size());
        insertUint16(len);
        for (char c : msg.game_name) buffer.push_back(static_cast<uint8_t>(c));
    } else if (opcode == CHANGE_CAR) {
        uint16_t len = static_cast<uint16_t>(msg.car_type.size());
        insertUint16(len);
        for (char c : msg.car_type) buffer.push_back(static_cast<uint8_t>(c));
    }
    return buffer;
}

std::vector<std::uint8_t> Protocol::encodeOpcode(std::uint8_t opcode) {
    buffer.clear();
    buffer.push_back(opcode);
    return buffer;
}

std::vector<std::uint8_t> Protocol::encodeServerMessage(ServerMessage& out) {
    buffer.clear();
    // Decidir qué payload serializar según opcode
    if (out.opcode == UPDATE_POSITIONS) {
        buffer.push_back(UPDATE_POSITIONS);
        buffer.push_back(static_cast<std::uint8_t>(out.positions.size()));
        for (auto & pos_update : out.positions) {
            insertInt(pos_update.player_id);
            insertFloat(pos_update.new_pos.new_X);
            insertFloat(pos_update.new_pos.new_Y);
            insertFloat(pos_update.new_pos.angle);
            insertInt(static_cast<int>(pos_update.new_pos.direction_x));
            insertInt(static_cast<int>(pos_update.new_pos.direction_y));
            // Enviar cantidad de checkpoints siguientes y sus posiciones (x,y floats)
            uint8_t next_count = static_cast<uint8_t>(pos_update.next_checkpoints.size());
            buffer.push_back(next_count);
            for (const auto &cp : pos_update.next_checkpoints) {
                insertFloat(cp.new_X);
                insertFloat(cp.new_Y);
                insertFloat(cp.angle);
            }
            // Enviar car_type como string (uint16 length + bytes)
            uint16_t carLen = static_cast<uint16_t>(pos_update.car_type.size());
            insertUint16(carLen);
            for (char c : pos_update.car_type) buffer.push_back(static_cast<uint8_t>(c));
        }
        return buffer;
    } else if (out.opcode == GAME_JOINED) {
        // Reusar formato existente de GameJoinedResponse para compatibilidad en el cliente
        buffer.push_back(GAME_JOINED);
        insertUint32(out.game_id);
        insertUint32(out.player_id);
        buffer.push_back(out.success ? 1 : 0);
        return buffer;
    } else if (out.opcode == GAMES_LIST) {
        buffer.push_back(GAMES_LIST);
        insertUint32(static_cast<uint32_t>(out.games.size()));
        for (auto &g : out.games) {
            insertUint32(g.game_id);
            insertUint32(g.player_count);
            uint16_t len = static_cast<uint16_t>(g.name.size());
            insertUint16(len);
            for (char c : g.name) buffer.push_back(static_cast<uint8_t>(c));
        }
        return buffer;
    } else {
        // Desconocido: no serializar payload, solo opcode
        buffer.push_back(out.opcode);
        return buffer;
    }
}

std::vector<std::uint8_t> Protocol::encodeGameJoinedResponse(const GameJoinedResponse& response) {
    buffer.clear();
    buffer.push_back(GAME_JOINED);
    insertUint32(response.game_id);
    insertUint32(response.player_id);
    buffer.push_back(response.success ? 1 : 0);
    return buffer;
}
