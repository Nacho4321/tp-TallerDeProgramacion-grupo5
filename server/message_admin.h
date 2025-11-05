#ifndef MESSAGE_ADMIN_H
#define MESSAGE_ADMIN_H
#include "../common/thread.h"
#include "client_handler.h"
#include "../common/queue.h"
#include <unordered_map>
#include "../common/Event.h"
class MessageAdmin : public Thread
{
private:
    Queue<ClientHandlerMessage> &global_inbox;
    std::unordered_map<int, Queue<Event>> &game_queues;
    std::mutex &game_queues_mutex;

public:
    explicit MessageAdmin(Queue<ClientHandlerMessage> &global_in, std::unordered_map<int, Queue<Event>> &game_qs, std::mutex &game_qs_mutex) : global_inbox(global_in), game_queues(game_qs), game_queues_mutex(game_qs_mutex) {}
    void run() override;
};
#endif