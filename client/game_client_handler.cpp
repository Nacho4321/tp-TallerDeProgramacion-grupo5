#include "game_client_handler.h"

GameClientHandler::GameClientHandler(Protocol& proto)
        : protocol(proto), incoming(), outgoing(), join_results(),
            sender(protocol, outgoing), receiver(protocol, incoming, join_results) {}

void GameClientHandler::start() {
    // iniciar sender y receiver
    sender.start();
    receiver.start();
}

void GameClientHandler::stop() {
    // parar ambos
    sender.stop();
    receiver.stop();
}

void GameClientHandler::join() {
    // esperar que terminen
    sender.join();
    receiver.join();
}

void GameClientHandler::send(const std::string& msg) {
    outgoing.push(msg);
}

bool GameClientHandler::try_receive(ServerMessage& out) {
    return incoming.try_pop(out);
}

void GameClientHandler::set_player_id(int32_t id) {
    sender.set_player_id(id);
}

void GameClientHandler::set_game_id(int32_t id) {
    sender.set_game_id(id);
}

bool GameClientHandler::create_game_blocking(uint32_t& out_game_id, uint32_t& out_player_id) {
    // Reset IDs to -1 before solicitar creación
    std::cout << "[Handler] Preparando CREATE_GAME..." << std::endl;
    sender.set_game_id(-1);
    sender.set_player_id(-1);
    send("create_game");
    std::cout << "[Handler] CREATE_GAME enviado, esperando respuesta..." << std::endl;
    try {
        ServerMessage resp = join_results.pop(); // bloquea hasta respuesta
        if (resp.opcode != GAME_JOINED) {
            std::cout << "[Handler] Ignorando mensaje no-GAME_JOINED durante create (opcode=" << int(resp.opcode) << ")" << std::endl;
            return false;
        }
        std::cout << "[Handler] Respuesta recibida: game_id=" << resp.game_id 
                  << " player_id=" << resp.player_id 
                  << " success=" << resp.success << std::endl;
        if (resp.success) {
            // Actualizamos sender con IDs asignados
            sender.set_game_id(static_cast<int32_t>(resp.game_id));
            sender.set_player_id(static_cast<int32_t>(resp.player_id));
            out_game_id = resp.game_id;
            out_player_id = resp.player_id;
            return true;
        }
        return false;
    } catch (const ClosedQueue&) {
        std::cerr << "[Handler] ERROR: Cola cerrada antes de recibir respuesta" << std::endl;
        return false; // cola cerrada -> fallo
    }
}

bool GameClientHandler::join_game_blocking(int32_t game_id_to_join, uint32_t& out_player_id) {
    // Seteamos game_id deseado antes de enviar join
    sender.set_game_id(game_id_to_join);
    send("join_game");
    try {
        ServerMessage resp = join_results.pop();
        if (resp.opcode != GAME_JOINED) {
            std::cout << "[Handler] Ignorando mensaje no-GAME_JOINED durante join (opcode=" << int(resp.opcode) << ")" << std::endl;
            return false;
        }
        if (resp.success) {
            // Ajustamos IDs reales devueltos
            sender.set_game_id(static_cast<int32_t>(resp.game_id));
            sender.set_player_id(static_cast<int32_t>(resp.player_id));
            out_player_id = resp.player_id;
            return true;
        }
        return false;
    } catch (const ClosedQueue&) {
        return false;
    }
}
