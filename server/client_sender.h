#ifndef CLIENT_SENDER_H
#define CLIENT_SENDER_H

#include "../common/thread.h"
#include "../common/protocol.h"
#include "../common/queue.h"
#include "../common/messages.h"

class ClientSender : public Thread {
	Protocol &protocol;
	Queue<ServerMessage> &outbox;

public:
	ClientSender(Protocol &proto, Queue<ServerMessage> &ob);
	void run() override;
};

#endif // CLIENT_SENDER_H