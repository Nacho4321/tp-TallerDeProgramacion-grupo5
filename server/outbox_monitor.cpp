#include "outbox_monitor.h"

void OutboxMonitor::add(int client_id, std::shared_ptr<Queue<ServerMessage>> q)
{
    std::lock_guard<std::mutex> lock(mtx);
    outboxes[client_id] = q;
}

void OutboxMonitor::remove(int client_id)
{
    std::lock_guard<std::mutex> lock(mtx);
    outboxes.erase(client_id);
}

void OutboxMonitor::remove_all()
{
    std::lock_guard<std::mutex> lock(mtx);
    outboxes.clear();
}

std::shared_ptr<Queue<ServerMessage>> OutboxMonitor::get(int id)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto it = outboxes.find(id);
    if (it != outboxes.end()) {
        return it->second;
    }
    return nullptr;
}
