#ifndef GAME_STATE_H
#define GAME_STATE_H

enum class GameState
{
    LOBBY,    // Esperando en lobby, que seleccionen autos
    STARTING, // Cuenta regresiva antes de que comience la carrera
    PLAYING   // Carrera en progreso
};

#endif 
