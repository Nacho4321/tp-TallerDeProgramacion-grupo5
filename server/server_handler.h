#ifndef SERVER_HANDLER_H
#define SERVER_HANDLER_H

#include "acceptor.h"
#include "outbox_monitor.h"
#include "client_handler_msg.h"
#include "../common/queue.h"

// ServerHandler: encapsula el arranque y ciclo de vida del servidor.
// - Construye el Acceptor con el puerto indicado
// - start(): inicia el hilo del acceptor
// - ~ServerHandler(): detiene y joinea automáticamente
// - try_receive(): intenta obtener un mensaje del inbox global
// - broadcast(): envía un ServerMessage a todos los clientes
class ServerHandler {
public:
    explicit ServerHandler(const char* port);
    ~ServerHandler();

    // No copiable ni movable (simplifica ownership del hilo)
    ServerHandler(const ServerHandler&) = delete;
    ServerHandler& operator=(const ServerHandler&) = delete;

    void start();
    void stop();

    bool try_receive(ClientHandlerMessage& out);
    void broadcast(const ServerMessage& msg);

private:
    Queue<ClientHandlerMessage> inbox;
    Queue<int> players;
    OutboxMonitor outboxes;
    Acceptor acceptor;
    bool started{false};
};

#endif // SERVER_HANDLER_H
