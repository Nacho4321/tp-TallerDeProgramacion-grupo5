#ifndef GAMELOOP_H
#define GAMELOOP_H
#include "thread.h"
#include <unordered_map>
#include "../common/eventloop.h"
#include "PlayerData.h"
#include "../common/messages.h"
#include "../server/outbox_monitor.h"
#include "../server/client_handler_msg.h"
#define INITIAL_ID 1
class GameLoop : public Thread
{
private:
    std::mutex players_map_mutex;
    std::unordered_map<int, PlayerData> players;
    Queue<Event> event_queue;
    EventLoop event_loop;
    bool started;
    OutboxMonitor &outbox_moitor;
    void init_players();
    int next_id;

public:
    explicit GameLoop(OutboxMonitor &outboxes) : players_map_mutex(), players(), event_queue(), event_loop(players_map_mutex, players, event_queue), started(false), outbox_moitor(outboxes), next_id(INITIAL_ID) {}
    void run() override;
    void start_game();
};
#endif