#include "gameloop.h"
#define MAX_PLAYERS 8
void GameLoop::run()
{
    init_players();
    event_loop.start();
    while (should_keep_running())
    {
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
    while (!started)
    {
        if (int(player_data.size()) < MAX_PLAYERS)
        {
            bool popped = game_clients.try_pop(id);
            if (popped)
            {
                player_data_mutex.lock();
                player_data[id] = Player{};
                player_data_mutex.unlock();
            }
        }
    }
}

void GameLoop::start_game()
{
    started = true;
}