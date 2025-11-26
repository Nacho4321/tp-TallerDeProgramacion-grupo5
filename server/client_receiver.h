#ifndef CLIENT_RECEIVER_H
#define CLIENT_RECEIVER_H

#include "../common/thread.h"
#include "../common/protocol.h"
#include "../common/queue.h"
#include "../common/messages.h"

#include "client_handler_msg.h"

class ClientReceiver : public Thread
{
    Protocol &protocol;
    int client_id;
    Queue<ClientHandlerMessage> &global_inbox;

public:
    ClientReceiver(Protocol &proto, int id, Queue<ClientHandlerMessage> &global_inbox);

    void run() override;
};
#endif