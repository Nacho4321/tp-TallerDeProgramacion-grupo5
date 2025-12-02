#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <memory>
#include <utility>
#include <vector>
#include <list>
#include "../common/protocol.h"
#include "../common/queue.h"
#include "../common/socket.h"
#include "../common/thread.h"

#include "client_handler.h"
#include "client_handler_msg.h"

// Forward declaration para evitar dependencia circular
class LobbyHandler;

class Acceptor : public Thread
{
    Socket acceptor;
    LobbyHandler &message_handler;
    std::vector<std::unique_ptr<ClientHandler>> clients;

public:
    explicit Acceptor(const char *port, LobbyHandler &msg_admin);
    explicit Acceptor(Socket &acc, LobbyHandler &msg_admin);

    void run() override;
    void stop() override;

private:
    void kill_all();
    void reap();
};

#endif // ACCEPTOR_H
