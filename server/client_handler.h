#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <memory>
#include <string>
#include <utility>

#include "../common/protocol.h"
#include "../common/queue.h"
#include "../common/socket.h"
#include "../common/thread.h"
#include "../common/messages.h"

#include "client_handler_msg.h"

// ---------------- ClientReceiver ----------------
class ClientReceiver : public Thread
{
    Protocol &protocol;
    int client_id;
    Queue<ClientHandlerMessage> &global_inbox;

public:
    ClientReceiver(Protocol &proto, int id, Queue<ClientHandlerMessage> &global_inbox);

    void run() override;
};

// ---------------- ClientSender ----------------
class ClientSender : public Thread
{
    Protocol &protocol;
    Queue<ServerResponse> &outbox;

public:
    ClientSender(Protocol &proto, Queue<ServerResponse> &ob);

    void run() override;
};

// ---------------- ClientHandler ----------------
class ClientHandler
{
    Protocol protocol;
    std::shared_ptr<Queue<ServerResponse>> outbox;
    Queue<ClientHandlerMessage> &global_inbox;
    ClientSender sender;
    int client_id;
    ClientReceiver receiver;

public:
    ClientHandler(Socket &&p, int id, Queue<ClientHandlerMessage> &global_inbox);

    void start();
    void stop();
    bool is_alive();
    void join();

    std::shared_ptr<Queue<ServerResponse>> get_outbox();
    int get_id();
};

#endif // CLIENT_HANDLER_H
