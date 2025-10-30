#ifndef GAMELOOP_H
#define GAMELOOP_H
#include "thread.h"
#include <unordered_map>
#include "../common/eventloop.h"
#include "PlayerData.h"
#include "../common/messages.h"
#include "../server/outbox_monitor.h"
class GameLoop : public Thread
{
private:
    // int game_id;
    std::mutex players_map_mutex;
    std::unordered_map<int, PlayerData> players;
    // unordered_map <player_id,Times> players;
    // std::list<Track> tracks limitar a tres;
    EventLoop event_loop;
    Queue<int> &game_clients;
    bool started;
    Queue<ClientMessage> &global_inbox;
    OutboxMonitor &outbox_moitor;
    void init_players();

public:
    explicit GameLoop(Queue<Event> &e_queue, Queue<int> &clientes, Queue<ClientMessage> &global_q, OutboxMonitor &outboxes) : players_map_mutex(), players(), event_loop(players_map_mutex, players, e_queue), game_clients(clientes), started(false), global_inbox(global_q), outbox_moitor(outboxes) {}
    void run() override;
    void start_game();
};
#endif