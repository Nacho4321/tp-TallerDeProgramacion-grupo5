#ifndef CLIENT_RECEIVER_H
#define CLIENT_RECEIVER_H

#include "../common/thread.h"
#include "../common/protocol.h"
#include "../common/queue.h"
#include "../common/messages.h"

#include "client_handler_msg.h"

// Forward declaration
class MessageHandler;

class ClientReceiver : public Thread
{
    Protocol &protocol;
    int client_id;
    MessageHandler &message_handler;
    std::shared_ptr<Queue<ServerMessage>> outbox;

public:
    ClientReceiver(Protocol &proto, int id, MessageHandler &msg_admin, std::shared_ptr<Queue<ServerMessage>> out);

    void run() override;
};
#endif