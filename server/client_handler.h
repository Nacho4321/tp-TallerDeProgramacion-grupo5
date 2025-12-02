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
#include "client_receiver.h"
#include "client_sender.h"

// Forward declaration
class LobbyHandler;

// ---------------- ClientHandler ----------------
class ClientHandler
{
    Protocol protocol;
    std::shared_ptr<Queue<ServerMessage>> outbox;
    LobbyHandler &message_handler;
    ClientSender sender;
    int client_id;
    ClientReceiver receiver;

    static int next_id;

public:
    ClientHandler(Socket &&p, LobbyHandler &msg_handler);
    ~ClientHandler();

    void start();
    void stop();
    bool is_alive();

    std::shared_ptr<Queue<ServerMessage>> get_outbox();
    int get_id();
};

#endif // CLIENT_HANDLER_H
