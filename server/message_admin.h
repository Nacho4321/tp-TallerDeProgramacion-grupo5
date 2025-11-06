#ifndef MESSAGE_ADMIN_H
#define MESSAGE_ADMIN_H
#include "../common/thread.h"
#include "client_handler.h"
#include "../common/queue.h"
#include <unordered_map>
#include "../common/Event.h"
#include <functional>
#include "game_monitor.h"
class MessageAdmin : public Thread
{
private:
    Queue<ClientHandlerMessage> &global_inbox;
    std::unordered_map<int, std::shared_ptr<Queue<Event>>> &game_queues;
    std::mutex &game_queues_mutex;
    GameMonitor &games_monitor;
    std::unordered_map<std::string, std::function<void(int &)>> cli_comm_dispatch;
    OutboxMonitor &outboxes;
    void init_dispatch();
    void create_game(int &client_id);
    void join_game(int &client_id);

public:
    explicit MessageAdmin(Queue<ClientHandlerMessage> &global_in, std::unordered_map<int, std::shared_ptr<Queue<Event>>> &game_qs, std::mutex &game_qs_mutex, GameMonitor &games_mon, OutboxMonitor &outbox) : global_inbox(global_in), game_queues(game_qs), game_queues_mutex(game_qs_mutex), games_monitor(games_mon), cli_comm_dispatch(), outboxes(outbox) {}
    void run() override;
    void handle_message();
};
#endif