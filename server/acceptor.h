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

class Acceptor : public Thread
{
    Socket acceptor;
    Queue<ClientHandlerMessage> &global_inbox;
    std::vector<std::unique_ptr<ClientHandler>> clients;
    OutboxMonitor &outbox_monitor;
    int next_id = 0;

public:
    explicit Acceptor(const char *port, Queue<ClientHandlerMessage> &global_inbox, OutboxMonitor &outboxes);
    explicit Acceptor(Socket &acc, Queue<ClientHandlerMessage> &global_inbox, OutboxMonitor &outboxes);

    void run() override;
    void stop() override;

private:
    void clear();
    void reap();
};

#endif // ACCEPTOR_H
