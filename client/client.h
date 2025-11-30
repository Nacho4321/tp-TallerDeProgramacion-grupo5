#ifndef CLIENT_H
#define CLIENT_H
#include "../common/protocol.h"
#include "../common/queue.h"
#include "input_handler.h"
#include "game_client_handler.h"
#include "game_connection.h"
#include "game_renderer.h"
#include <SDL2pp/SDL2pp.hh>
#include <unordered_map>
#include <string>
#include <memory>

// Agrego esto para q no tengamos que apretar teclas al unirnos a una partida, pero siga funcionadno esa opcion
enum class StartMode
{
    NORMAL,      // espera comandos (create/join)
    AUTO_CREATE, // crea partida autoamticamente al inicio
    AUTO_JOIN,   // se une a partida automaticamente al inicio
    FROM_LOBBY   // recibe conexion ya establecida desde el lobby Qt
};

class Client
{
private:
    std::unique_ptr<GameConnection> connection_;
    
    // conexion creada internamente (para pruebas)
    std::unique_ptr<Protocol> owned_protocol_;
    std::unique_ptr<GameClientHandler> owned_handler_;
    
    // puntero al handler activo
    GameClientHandler* active_handler_;
    
    bool connected;
    InputHandler handler;

    std::unordered_map<std::string, int> car_type_map = {
        {GREEN_CAR, 0},
        {RED_SQUARED_CAR, 1},
        {RED_SPORTS_CAR, 2},
        {LIGHT_BLUE_CAR, 3},
        {RED_JEEP_CAR, 4},
        {PURPLE_TRUCK, 5},
        {LIMOUSINE_CAR, 6}};
    GameRenderer game_renderer;

    // IDs asignados por el servidor para identificar mi partida/jugador actuales
    uint32_t my_game_id = 0;   // 0 => no asignado aún
    int32_t my_player_id = -1; // -1 => no asignado aún
    int32_t original_player_id = -1; 

    // Configuración de inicio
    StartMode start_mode;
    int auto_join_game_id;
    std::string auto_create_game_name;

    int SPECTATOR_MODE = -1;
    
    // Helper para inicializar conexion como antes
    void initLegacyConnection(const char* address, const char* port);

public:
    explicit Client(const char *address, const char *port,
                    StartMode mode = StartMode::NORMAL,
                    int join_game_id = -1,
                    const std::string &game_name = "");
    
    // constructor nuevo
    explicit Client(std::unique_ptr<GameConnection> connection);
    
    ~Client();
    void start();
    bool isConnected() const { return connected; }
};
#endif