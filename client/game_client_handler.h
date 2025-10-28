#ifndef GAME_CLIENT_HANDLER_H
#define GAME_CLIENT_HANDLER_H

#include "../common/protocol.h"
#include "../common/queue.h"
#include "game_client_sender.h"
#include "game_client_receiver.h"

// Encapsula sender/receiver y las colas de entrada/salida.
// Provee una interfaz simple: send(...) y try_receive(...),
// además de los métodos de ciclo de vida start/stop/join.
class GameClientHandler {
private:
    Protocol& protocol;
    Queue<DecodedMessage> incoming;
    Queue<std::string> outgoing;

    GameClientSender sender;
    GameClientReceiver receiver;

public:
    explicit GameClientHandler(Protocol& proto);

    // No copy
    GameClientHandler(const GameClientHandler&) = delete;
    GameClientHandler& operator=(const GameClientHandler&) = delete;

    // Movimiento no permitido (podríamos implementarlo si hace falta)
    GameClientHandler(GameClientHandler&&) = delete;
    GameClientHandler& operator=(GameClientHandler&&) = delete;

    // Ciclo de vida
    void start();
    void stop();
    void join();

    // Interfaz simple
    void send(const std::string& msg);
    // try_receive devuelve true si obtuvo un mensaje no bloqueante
    bool try_receive(DecodedMessage& out);
};

#endif // GAME_CLIENT_HANDLER_H
