#ifndef OUTBOX_MONITOR_H
#define OUTBOX_MONITOR_H
#include <memory>
#include <mutex>
#include <unordered_map>

#include "../common/queue.h"
#include "../common/messages.h"

class OutboxMonitor
{
private:
    std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> outboxes;
    std::mutex mtx;

public:
    void add(int client_id, std::shared_ptr<Queue<ServerMessage>> q);
    void remove(int client_id);
    void broadcast(const ServerMessage &msg);
    std::shared_ptr<Queue<ServerMessage>> get_cliente_queue(int id);
    void remove_all();
};

#endif
