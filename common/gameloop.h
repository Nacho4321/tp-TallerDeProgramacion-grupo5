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
    std::unordered_map<int, std::shared_ptr<Queue<ServerMessage>>> players_messanger;
    std::shared_ptr<Queue<Event>> event_queue;
    EventLoop event_loop;
    bool started;
    int next_id;

public:
    explicit GameLoop(std::shared_ptr<Queue<Event>> events) : players_map_mutex(), players(), players_messanger(), event_queue(events), event_loop(players_map_mutex, players, event_queue), started(false), next_id(INITIAL_ID) {}
    void run() override;
    void start_game();
    void add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox);
};
#endif