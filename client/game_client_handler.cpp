#include "game_client_handler.h"

GameClientHandler::GameClientHandler(Protocol& proto)
    : protocol(proto), incoming(), outgoing(),
      sender(protocol, outgoing), receiver(protocol, incoming) {}

void GameClientHandler::start() {
    // iniciar sender y receiver
    sender.start();
    receiver.start();
}

void GameClientHandler::stop() {
    // parar ambos: sender cerrará su cola, receiver cerrará el socket
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

bool GameClientHandler::try_receive(DecodedMessage& out) {
    return incoming.try_pop(out);
}
