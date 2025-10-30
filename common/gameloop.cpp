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

        // esperar a que den start
        // abrir taller
        // jugar primer carrera
        // mostrar_tiempos
        // abrir taller
        // jugar segunda carrera
        // mostrar tiempos
        // abrir taller
        // jugar tercera carrera
        // mostrar tiempos
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
            players[id] = PlayerData{MOVE_UP_RELEASED_STR, CarInfo{"lambo", 15, 15, 15}, Position{960, 540, not_horizontal, not_vertical}};
        }
    }
}

void GameLoop::start_game()
{
    started = true;
}