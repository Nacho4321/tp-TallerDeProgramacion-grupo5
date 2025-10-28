#include "game_client_receiver.h"
#include <iostream>

GameClientReceiver::GameClientReceiver(Protocol& proto, Queue<DecodedMessage>& messages) :
    protocol(proto), incoming_messages(messages) {}

void GameClientReceiver::run() {
    try {
        while (should_keep_running()) {
            DecodedMessage msg = protocol.receiveMessage();
            if (msg.cmd.empty())
                break;  // EOF o desconexión
            incoming_messages.push(std::move(msg));
        }
    } catch (const std::exception& e) {
        std::cerr << "[Game Client Receiver] Exception: " << e.what() << std::endl;
        if (should_keep_running()) {
            throw;  // Solo propagamos el error si no estábamos parando el hilo
        }
    }
}

void GameClientReceiver::stop() {
    Thread::stop();  // Marca el flag de detención
    protocol.shutdown();  // Cierra el socket para interrumpir receiveMessage()
}