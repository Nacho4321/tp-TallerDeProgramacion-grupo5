#ifndef GAME_CLIENT_RECEIVER_H
#define GAME_CLIENT_RECEIVER_H

#include <memory>
#include "../common/queue.h"
#include "../common/socket.h"
#include "../common/thread.h"
#include "../common/protocol.h"

// TODO: -Mover a client/ junto con el sender

class GameClientReceiver : public Thread {
private:
    Protocol& protocol;
    Queue<DecodedMessage>& incoming_messages;

public:
    explicit GameClientReceiver(Protocol& proto, Queue<DecodedMessage>& messages);
    
    void run() override;
    void stop() override;  // Sobreescribimos stop para cerrar el socket
    
    // No permitimos la copia del receptor
    GameClientReceiver(const GameClientReceiver&) = delete;
    GameClientReceiver& operator=(const GameClientReceiver&) = delete;

    // Pero sí permitimos el movimiento
    GameClientReceiver(GameClientReceiver&&) = default;
    GameClientReceiver& operator=(GameClientReceiver&&) = default;
};

#endif // CLIENT_RECEIVER_H