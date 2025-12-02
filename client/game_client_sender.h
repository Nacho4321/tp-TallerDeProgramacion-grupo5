#ifndef GAME_CLIENT_SENDER_H
#define GAME_CLIENT_SENDER_H

#include <memory>
#include "../common/queue.h"
#include "../common/socket.h"
#include "../common/thread.h"
#include "../common/protocol.h"


class GameClientSender : public Thread {
private:
    Protocol& protocol;
    Queue<std::string>& outgoing_messages;
    int32_t player_id{-1};
    int32_t game_id{-1};

public:
    explicit GameClientSender(Protocol& proto, Queue<std::string>& messages);
    
    void set_player_id(int32_t id);
    void set_game_id(int32_t id);
    
    void run() override;
    void stop() override;  
    
    GameClientSender(const GameClientSender&) = delete;
    GameClientSender& operator=(const GameClientSender&) = delete;

    GameClientSender(GameClientSender&&) = default;
    GameClientSender& operator=(GameClientSender&&) = default;
};

#endif // CLIENT_SENDER_H