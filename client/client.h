#ifndef CLIENT_H
#define CLIENT_H
#include "../common/protocol.h"
#include "../common/queue.h"
#include "input_handler.h"
#include "game_client_handler.h"
#include "game_renderer.h"
#include <SDL2pp/SDL2pp.hh>

// Modos de inicio del cliente
enum class StartMode {
    NORMAL,      // Modo interactivo normal (espera comandos del usuario)
    AUTO_CREATE, // Crea partida automáticamente al inicio
    AUTO_JOIN    // Se une a partida automáticamente al inicio
};

class Client
{
private:
    Protocol protocol;
    Socket ini_protocol(const char *address, const char *port)
    {
        Socket sock(address, port);
        return sock;
    }
    bool connected;
    InputHandler handler;
    
    // Handler que encapsula sender/receiver y colas
    GameClientHandler handler_core;

    GameRenderer game_renderer;

    // IDs asignados por el servidor para identificar mi partida/jugador actuales
    uint32_t my_game_id = 0;     // 0 => no asignado aún
    int32_t my_player_id = -1;   // -1 => no asignado aún

    // Configuración de inicio
    StartMode start_mode;
    int auto_join_game_id;

public:
    explicit Client(const char *address, const char *port, 
                   StartMode mode = StartMode::NORMAL, 
                   int join_game_id = -1);
    ~Client();
    void start();
    bool isConnected() const { return connected; }
};
#endif