#ifndef GAMELOOP_H
#define GAMELOOP_H
#include "thread.h"
#include <unordered_map>
#include "../common/eventloop.h"
#include "Player.h"
#include <mutex>
class GameLoop : public Thread
{
private:
    // int game_id;
    std::mutex player_data_mutex;
    std::unordered_map<int, Player>
        player_data;
    // unordered_map <player_id,Times> players;
    // std::list<Track> tracks limitar a tres;
    EventLoop event_loop;
    Queue<int> &game_clients;
    bool started;
    void init_players();

public:
    explicit GameLoop(Queue<Event> &e_queue, Queue<int> &clientes) : player_data_mutex(), event_loop(e_queue), game_clients(clientes), started(false) {}
    void run() override;
    void start_game();
};
#endif