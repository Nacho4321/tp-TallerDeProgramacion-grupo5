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
#include "outbox_monitor.h"

// Forward declaration para evitar dependencia circular
class MessageHandler;

class Acceptor : public Thread
{
    Socket acceptor;
    MessageHandler &message_handler;
    std::vector<std::unique_ptr<ClientHandler>> clients;
    OutboxMonitor &outbox_monitor;
    int next_id = 0;

public:
    explicit Acceptor(const char *port, MessageHandler &msg_admin, OutboxMonitor &outboxes);
    explicit Acceptor(Socket &acc, MessageHandler &msg_admin, OutboxMonitor &outboxes);

    void run() override;
    void stop() override;

private:
    void clear();
    void reap();
};

#endif // ACCEPTOR_H
