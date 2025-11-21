#include "game_client_sender.h"
#include <iostream>

GameClientSender::GameClientSender(Protocol& proto, Queue<std::string>& messages) :
    protocol(proto), outgoing_messages(messages) {}

void GameClientSender::run() {
    try {
        while (should_keep_running()) {
            std::string msg;
            try {
                msg = outgoing_messages.pop();  // bloqueante
            } catch (const ClosedQueue&) {
                // La cola fue cerrada: salimos del loop
                break;
            }
            if (!should_keep_running()) break;  // Verificar antes de enviar
            ClientMessage client_msg;
            client_msg.cmd = msg;
            // Adaptación al nuevo protocolo: incluir IDs
            client_msg.player_id = player_id;
            client_msg.game_id = game_id;

            // Fallback: si es join_game con id en el string y aún no tenemos game_id seteado,
            // parsear el id del comando y usarlo en este envío.
            if (client_msg.cmd.rfind(JOIN_GAME_STR, 0) == 0) {
                // buscar un espacio y parsear el número siguiente
                size_t sp = client_msg.cmd.find(' ');
                if (sp != std::string::npos && sp + 1 < client_msg.cmd.size()) {
                    std::string id_str = client_msg.cmd.substr(sp + 1);
                    try {
                        int parsed = std::stoi(id_str);
                        client_msg.game_id = parsed;
                    } catch (...) {
                        // si falla, dejamos el game_id como está
                    }
                }
                // Normalizar el comando a solo "join_game" para el mapeo de opcode
                client_msg.cmd = JOIN_GAME_STR;
            }
            // Parse create_game <nombre>
            if (client_msg.cmd.rfind(CREATE_GAME_STR, 0) == 0) {
                size_t sp = client_msg.cmd.find(' ');
                if (sp != std::string::npos && sp + 1 < client_msg.cmd.size()) {
                    client_msg.game_name = client_msg.cmd.substr(sp + 1);
                    client_msg.cmd = CREATE_GAME_STR; // normalizar
                } else {
                    client_msg.game_name = ""; // nombre vacío => server asigna por defecto
                }
            }
            if (client_msg.cmd == GET_GAMES_STR) {
                // no payload extra
            }
            // DEBUG
            std::cout << "[Sender] Enviando cmd='" << client_msg.cmd
                      << "' player_id=" << client_msg.player_id
                      << " game_id=" << client_msg.game_id << std::endl;
            protocol.sendMessage(client_msg);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Game Client Sender] Exception: " << e.what() << std::endl;
        if (should_keep_running()) {
            throw;  // Solo propagamos el error si no estábamos parando el hilo
        }
    }
}

void GameClientSender::stop() {
    Thread::stop(); 
    outgoing_messages.close();  // Cierra la cola para interrumpir pop()
}