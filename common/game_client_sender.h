#ifndef GAME_CLIENT_SENDER_H
#define GAME_CLIENT_SENDER_H

#include <memory>
#include "queue.h"
#include "socket.h"
#include "thread.h"
#include "protocol.h"

class GameClientSender : public Thread {
private:
    Protocol& protocol;
    Queue<std::string>& outgoing_messages;

public:
    explicit GameClientSender(Protocol& proto, Queue<std::string>& messages);
    
    void run() override;
    void stop() override;  // Sobreescribimos stop para cerrar la cola de mensajes
    
    // No permitimos la copia del sender
    GameClientSender(const GameClientSender&) = delete;
    GameClientSender& operator=(const GameClientSender&) = delete;

    // Pero sí permitimos el movimiento
    GameClientSender(GameClientSender&&) = default;
    GameClientSender& operator=(GameClientSender&&) = default;
};

#endif // CLIENT_SENDER_H