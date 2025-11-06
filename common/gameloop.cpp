#include "gameloop.h"
#define MAX_PLAYERS 8
#define INITIAL_X_POS 960
#define INITIAL_Y_POS 540
#define INITIAL_SPEED 0.0008
#define FULL_LOBBY_MSG "can't join lobby, maximum players reached"
void GameLoop::run()
{
    event_loop.start();
    while (should_keep_running())
    {
        if (int(players.size()) > 0)
        {
            std::vector<PlayerPositionUpdate> broadcast;
            players_map_mutex.lock();
            for (auto &[id, player_data] : players)
            {
                Position &pos = player_data.position;
                pos.new_X += float(pos.direction_x) * player_data.car.speed;
                pos.new_Y += float(pos.direction_y) * player_data.car.speed;
                PlayerPositionUpdate update = PlayerPositionUpdate{id, pos};
                broadcast.push_back(update);
            }
            players_map_mutex.unlock();
            ServerMessage msg = {broadcast};

            broadcast_positions(msg);
        }
    }

    event_loop.stop();
    event_loop.join();
}

void GameLoop::add_player(int id, std::shared_ptr<Queue<ServerMessage>> player_outbox)
{
    if (int(players.size()) == 0)
    {
        players[id] = PlayerData{
            MOVE_UP_RELEASED_STR, CarInfo{"lambo", INITIAL_SPEED, INITIAL_SPEED, INITIAL_SPEED}, Position{INITIAL_X_POS, INITIAL_Y_POS, not_horizontal, not_vertical}};
        players_messanger[id] = player_outbox;
    }
    else if (int(players.size()) < MAX_PLAYERS)
    {
        float dir_x = players[int(players.max_size())].position.new_X + 30;
        float dir_y = players[int(players.max_size())].position.new_Y;
        players[id] = PlayerData{
            MOVE_UP_RELEASED_STR, CarInfo{"lambo", INITIAL_SPEED, INITIAL_SPEED, INITIAL_SPEED}, Position{dir_x, dir_y, not_horizontal, not_vertical}};
        players_messanger[id] = player_outbox;
    }
    else
    {
        std::cout << FULL_LOBBY_MSG << std::endl;
    }
}

void GameLoop::start_game()
{
    started = true;
}

void GameLoop::broadcast_positions(ServerMessage &msg)
{
    for (auto &[id, queue] : players_messanger)
    {
        queue->push(msg);
    }
}
