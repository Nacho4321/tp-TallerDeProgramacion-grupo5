#include "gameloop.h"

void GameLoop::run()
{
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