#include "outbox_monitor.h"

void OutboxMonitor::add(std::shared_ptr<Queue<OutgoingMessage>> q) {
    std::lock_guard<std::mutex> lock(mtx);
    outboxes.push_back(q);
}

void OutboxMonitor::remove(std::shared_ptr<Queue<OutgoingMessage>> q) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto it = outboxes.begin(); it != outboxes.end();) {
        if (*it == q)
            it = outboxes.erase(it);
        else
            ++it;
    }
}

void OutboxMonitor::broadcast(const OutgoingMessage& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& q: outboxes) {
        if (!q->try_push(msg)) {
            // cliente desconectado o cola cerrada, ignorar
        }
    }
}
