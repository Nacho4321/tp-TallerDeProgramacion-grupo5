#ifndef GAME_CLIENT_RECEIVER_H
#define GAME_CLIENT_RECEIVER_H

#include <memory>
#include "../common/queue.h"
#include "../common/socket.h"
#include "../common/thread.h"
#include "../common/protocol.h"


class GameClientReceiver : public Thread {
private:
    Protocol& protocol;
    Queue<ServerMessage>& incoming_messages;
    Queue<GameJoinedResponse>& join_results;

public:
    explicit GameClientReceiver(Protocol& proto, Queue<ServerMessage>& messages, Queue<GameJoinedResponse>& joins);
    
    void run() override;
    void stop() override;  
    
    GameClientReceiver(const GameClientReceiver&) = delete;
    GameClientReceiver& operator=(const GameClientReceiver&) = delete;

    GameClientReceiver(GameClientReceiver&&) = default;
    GameClientReceiver& operator=(GameClientReceiver&&) = default;
};

#endif // CLIENT_RECEIVER_H