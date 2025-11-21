#ifndef GAME_CLIENT_HANDLER_H
#define GAME_CLIENT_HANDLER_H

#include "../common/protocol.h"
#include "../common/queue.h"
#include "game_client_sender.h"
#include "game_client_receiver.h"

// Encapsula sender/receiver y las colas de entrada/salida.
// Provee una interfaz simple: send(...) y try_receive(...),
// además de los métodos de ciclo de vida start/stop/join.
class GameClientHandler {
private:
    Protocol& protocol;
    Queue<ServerMessage> incoming;
    Queue<std::string> outgoing;
    Queue<ServerMessage> join_results;

    GameClientSender sender;
    GameClientReceiver receiver;

public:
    explicit GameClientHandler(Protocol& proto);

    GameClientHandler(const GameClientHandler&) = delete;
    GameClientHandler& operator=(const GameClientHandler&) = delete;

    GameClientHandler(GameClientHandler&&) = delete;
    GameClientHandler& operator=(GameClientHandler&&) = delete;

    // Ciclo de vida
    void start();
    void stop();
    void join();

    // Interfaz simple
    void send(const std::string& msg);
    // try_receive devuelve true si obtuvo un mensaje no bloqueante
    bool try_receive(ServerMessage& out);

    // IDs (por ahora setters simples para que el cliente los configure)
    void set_player_id(int32_t id);
    void set_game_id(int32_t id);

    // Lobby helpers que bloquean hasta recibir GAME_JOINED del servidor
    // Devuelven true si success==true en la respuesta; en caso afirmativo actualizan IDs internos
    bool create_game_blocking(uint32_t& out_game_id, uint32_t& out_player_id);
    bool join_game_blocking(int32_t game_id_to_join, uint32_t& out_player_id);
    
    // Solicita lista de partidas y bloquea hasta recibir GAMES_LIST
    std::vector<ServerMessage::GameSummary> get_games_blocking();
};

#endif // GAME_CLIENT_HANDLER_H
