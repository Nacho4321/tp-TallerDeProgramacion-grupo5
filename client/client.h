#ifndef CLIENT_H
#define CLIENT_H
#include "../common/protocol.h"
#include "../common/queue.h"
#include "input_handler.h"
#include "game_client_handler.h"
#include "game_renderer.h"
#include <SDL2pp/SDL2pp.hh>

// Agrego esto para q no tengamos que apretar teclas al unirnos a una partida, pero siga funcionadno esa opcion
enum class StartMode {
    NORMAL,      // espera comandos (create/join)
    AUTO_CREATE, // crea partida autoamticamente al inicio
    AUTO_JOIN    // se une a partida automaticamente al inicio
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
    std::string auto_create_game_name;

public:
    explicit Client(const char *address, const char *port, 
                   StartMode mode = StartMode::NORMAL, 
                   int join_game_id = -1,
                   const std::string& game_name = "");
    ~Client();
    void start();
    bool isConnected() const { return connected; }
};
#endif