#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <memory>
#include <string>
#include <utility>

#include "../common/protocol.h"
#include "../common/queue.h"
#include "../common/socket.h"
#include "../common/thread.h"

#include "messages.h"


// ---------------- ClientReceiver ----------------
class ClientReceiver: public Thread {
    Protocol& protocol;
    int client_id;
    Queue<IncomingMessage>& global_inbox;

public:
    ClientReceiver(Protocol& proto, int id, Queue<IncomingMessage>& global_inbox);

    void run() override;

private:
    IncomingMessage make_message_from_decoded(const DecodedMessage& cmd);
};

// ---------------- ClientSender ----------------
class ClientSender: public Thread {
    Protocol& protocol;
    Queue<OutgoingMessage>& outbox;

public:
    ClientSender(Protocol& proto, Queue<OutgoingMessage>& ob);

    void run() override;
};

// ---------------- ClientHandler ----------------
class ClientHandler {
    Protocol protocol;
    std::shared_ptr<Queue<OutgoingMessage>> outbox;
    Queue<IncomingMessage>& global_inbox;
    ClientSender sender;
    int client_id;
    ClientReceiver receiver;

public:
    ClientHandler(Socket&& p, int id, Queue<IncomingMessage>& global_inbox);

    void start();
    void stop();
    bool is_alive();
    void join();

    std::shared_ptr<Queue<OutgoingMessage>> get_outbox();
    int get_id();
};

#endif  // CLIENT_HANDLER_H
