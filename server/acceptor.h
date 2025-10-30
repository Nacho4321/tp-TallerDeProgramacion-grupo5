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
#include "outbox_monitor.h"

// TODO: -Usar monitor y dejar de usar mutexes en el acceptor
//       -Dejar al acceptor con unica responsabilidad, q deje de hacer broadcast y demas

class Acceptor : public Thread
{
    Socket acceptor;
    Queue<ClientMessage> &global_inbox;
    std::vector<std::unique_ptr<ClientHandler>> clients;
    Queue<int> &game_clients;
    OutboxMonitor &outbox_monitor;
    int next_id = 0;

public:
    explicit Acceptor(const char *port, Queue<ClientMessage> &global_inbox, Queue<int> &clients, OutboxMonitor &outboxes);
    explicit Acceptor(Socket &acc, Queue<ClientMessage> &global_inbox, Queue<int> &client, OutboxMonitor &outboxes);

    void run() override;
    void stop() override;

    void broadcast(const ServerMessage &msg);

private:
    void clear();
    void reap();
};

#endif // ACCEPTOR_H
