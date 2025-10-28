#ifndef GAMELOOP_H
#define GAMELOOP_H
#include "thread.h"
#include "../common/eventloop.h"
class GameLoop : public Thread
{
private:
    // int game_id;
    // unordered_map <player_id,Player> players;
    // unordered_map <player_id,Times> players;
    // std::list<Track> tracks limitar a tres;
    EventLoop event_loop;

public:
    explicit GameLoop(Queue<Event> &e_queue) : event_loop(e_queue) {}
    void run() override;
};
#endif