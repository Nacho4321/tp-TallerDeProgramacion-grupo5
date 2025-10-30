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
class ClientReceiver : public Thread
{
    Protocol &protocol;
    int client_id;
    Queue<ClientMessage> &global_inbox;

public:
    ClientReceiver(Protocol &proto, int id, Queue<ClientMessage> &global_inbox);

    void run() override;

private:
    ClientMessage make_message_from_decoded(const DecodedMessage &cmd);
};

// ---------------- ClientSender ----------------
class ClientSender : public Thread
{
    Protocol &protocol;
    Queue<ServerMessage> &outbox;

public:
    ClientSender(Protocol &proto, Queue<ServerMessage> &ob);

    void run() override;
};

// ---------------- ClientHandler ----------------
class ClientHandler
{
    Protocol protocol;
    std::shared_ptr<Queue<ServerMessage>> outbox;
    Queue<ClientMessage> &global_inbox;
    ClientSender sender;
    int client_id;
    ClientReceiver receiver;

public:
    ClientHandler(Socket &&p, int id, Queue<ClientMessage> &global_inbox);

    void start();
    void stop();
    bool is_alive();
    void join();

    std::shared_ptr<Queue<ServerMessage>> get_outbox();
    int get_id();
};

#endif // CLIENT_HANDLER_H
