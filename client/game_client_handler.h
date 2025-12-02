#ifndef GAME_CLIENT_HANDLER_H
#define GAME_CLIENT_HANDLER_H

#include "../common/protocol.h"
#include "../common/queue.h"
#include "game_client_sender.h"
#include "game_client_receiver.h"

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

    void start();
    void stop();
    void join();

    void send(const std::string& msg);
    bool try_receive(ServerMessage& out);

    void set_player_id(int32_t id);
    void set_game_id(int32_t id);

    bool create_game_blocking(uint32_t& out_game_id, uint32_t& out_player_id, uint8_t& out_map_id, const std::string& game_name = "", uint8_t map_id = 0);
    bool join_game_blocking(int32_t game_id_to_join, uint32_t& out_player_id, uint8_t& out_map_id);
    
    std::vector<ServerMessage::GameSummary> get_games_blocking();
    
    bool wait_for_game_started(int timeout_ms = 100); 
};

#endif // GAME_CLIENT_HANDLER_H
