#ifndef OUTBOX_MONITOR_H
#define OUTBOX_MONITOR_H
#include <memory>
#include <mutex>
#include <vector>

#include "../common/queue.h"

#include "messages.h"

class OutboxMonitor {
private:
    std::vector<std::shared_ptr<Queue<OutgoingMessage>>> outboxes;
    std::mutex mtx;

public:
    void add(std::shared_ptr<Queue<OutgoingMessage>> q);
    void remove(std::shared_ptr<Queue<OutgoingMessage>> q);
    void broadcast(const OutgoingMessage& msg);
};

#endif
