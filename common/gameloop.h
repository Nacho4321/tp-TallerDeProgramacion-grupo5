#ifndef GAMELOOP_H
#define GAMELOOP_H
#include "thread.h"
#include <unordered_map>
#include "../common/eventloop.h"
#include "PlayerData.h"
#include "../common/messages.h"
#include "../server/outbox_monitor.h"
#include "../server/client_handler_msg.h"
class GameLoop : public Thread
{
private:
    std::mutex players_map_mutex;
    std::unordered_map<int, PlayerData> players;
    Queue<ClientHandlerMessage> &global_inbox;
    EventLoop event_loop;
    Queue<int> &game_clients;
    bool started;
    OutboxMonitor &outbox_moitor;
    void init_players();
    void update_player_positions();

public:
    explicit GameLoop(Queue<int> &clientes, Queue<ClientHandlerMessage> &global_q, OutboxMonitor &outboxes) : players_map_mutex(), players(), global_inbox(global_q), event_loop(players_map_mutex, players, global_inbox), game_clients(clientes), started(false), outbox_moitor(outboxes) {}
    void run() override;
    void start_game();
};
#endif