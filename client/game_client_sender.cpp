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