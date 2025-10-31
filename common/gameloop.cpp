#include "gameloop.h"
#define MAX_PLAYERS 8
void GameLoop::run()
{
    event_loop.start();
    while (should_keep_running())
    {
        if (!started)
        {
            init_players();
        }
        else
        {
            update_player_positions();
            std::vector<PlayerPositionUpdate> broadcast;
            players_map_mutex.lock();
            for (auto &player : players)
            {
                int id = player.first;
                Position pos = player.second.position;
                PlayerPositionUpdate update = PlayerPositionUpdate{id, pos};
                broadcast.push_back(update);
            }
            players_map_mutex.unlock();
            ServerMessage msg = {broadcast};
            outbox_moitor.broadcast(msg);
        }
    }
    event_loop.stop();
    event_loop.join();
}

void GameLoop::init_players()
{
    int id;

    if (int(players.size()) < MAX_PLAYERS)
    {
        bool popped = game_clients.try_pop(id);
        if (popped)
        {
            if (id > 0)
            {
                float dir_x = players[id - 1].position.new_X + 30;
                float dir_y = players[id - 1].position.new_Y;
                players[id] = PlayerData{
                    MOVE_UP_RELEASED_STR, CarInfo{"lambo", 5, 5, 5}, Position{dir_x, dir_y, not_horizontal, not_vertical}};
            }
            else
            {
                players[id] = PlayerData{
                    MOVE_UP_RELEASED_STR, CarInfo{"lambo", 5, 5, 5}, Position{960, 540, not_horizontal, not_vertical}};
            }
        }
    }
}

void GameLoop::start_game()
{
    started = true;
}

void GameLoop::update_player_positions()
{
    std::lock_guard<std::mutex> lock(players_map_mutex);

    for (auto &[id, player_data] : players)
    {
        Position &pos = player_data.position;
        pos.new_X += float(pos.direction_x) * player_data.car.speed;
        pos.new_Y += float(pos.direction_y) * player_data.car.speed;
    }
}