#include "game_client_receiver.h"
#include <iostream>

GameClientReceiver::GameClientReceiver(Protocol& proto, Queue<ServerMessage>& messages, Queue<ServerMessage>& joins) :
    protocol(proto), incoming_messages(messages), join_results(joins) {}

void GameClientReceiver::run() {
    try {
        while (should_keep_running()) {
            ServerMessage positionsMsg;
            GameJoinedResponse joinResp{};
            uint8_t opcode = 0;
            bool ok = protocol.receiveAnyServerPacket(positionsMsg, joinResp, opcode);
            if (!ok) {
                break;
            }
            if (opcode == GAME_JOINED) {
                ServerMessage m; 
                m.opcode = GAME_JOINED; 
                m.game_id = joinResp.game_id; 
                m.player_id = joinResp.player_id; 
                m.success = joinResp.success;
                m.map_id = joinResp.map_id;
                join_results.push(std::move(m));
            } else if (opcode == UPDATE_POSITIONS) {
                if (!positionsMsg.positions.empty()) {
                    incoming_messages.push(std::move(positionsMsg));
                }
            } else if (opcode == GAMES_LIST) {
                incoming_messages.push(std::move(positionsMsg));
            } else if (opcode == GAME_STARTED) {
                ServerMessage m;
                m.opcode = GAME_STARTED;
                join_results.push(std::move(m)); 
                incoming_messages.push(std::move(positionsMsg));
            } else if (opcode == STARTING_COUNTDOWN) {
                ServerMessage m;
                m.opcode = GAME_STARTED;
                join_results.push(std::move(m)); 
                incoming_messages.push(std::move(positionsMsg));
            } else if (opcode == RACE_TIMES) {
                incoming_messages.push(std::move(positionsMsg));
            } else if (opcode == TOTAL_TIMES) {
                incoming_messages.push(std::move(positionsMsg));
            } else {
                // Paquete desconocido: ignorar
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Game Client Receiver] Exception: " << e.what() << std::endl;
        if (should_keep_running()) {
            throw;
        }
    }
    join_results.close();
    incoming_messages.close();
}

void GameClientReceiver::stop() {
    Thread::stop();  // Marca el flag de detención
    protocol.shutdown();  // Cierra el socket para interrumpir receiveMessage()
}