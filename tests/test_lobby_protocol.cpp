#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <string>

#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/constants.h"
#include "../common/messages.h"

static const char *TEST_PORT = "50300";

// ================================================================
// TEST: Cliente envía CREATE_GAME y servidor responde con GAME_JOINED
// ================================================================
TEST(LobbyProtocolTest, CreateGameFlow)
{
    std::thread server_thread([&]() {
        Socket acceptor(TEST_PORT);
        Socket peer = acceptor.accept();
        Protocol proto(std::move(peer));

        // Servidor espera 
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // El servidor responde
        GameJoinedResponse response;
        response.game_id = 1;
        response.player_id = 100;
        response.success = true;
        
        proto.sendMessage(response);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cliente se conecta y envía CREATE_GAME
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    
    ClientMessage create_msg;
    create_msg.cmd = "create_game";
    proto_client.sendMessage(create_msg);
    
    // Cliente espera respuesta
    GameJoinedResponse resp = proto_client.receiveGameJoined();
    
    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.game_id, 1);
    EXPECT_EQ(resp.player_id, 100);

    server_thread.join();
}

// ================================================================
// TEST: Cliente envía JOIN_GAME con ID y servidor responde
// ================================================================
TEST(LobbyProtocolTest, JoinGameFlow)
{
    std::thread server_thread([&]() {
        Socket acceptor(TEST_PORT);
        Socket peer = acceptor.accept();
        Protocol proto(std::move(peer));

        // Servidor recibe el mensaje de JOIN_GAME
        ClientMessage msg = proto.receiveClientMessage();
        
        // Verificar que el comando es correcto y el ID es 42
        EXPECT_EQ(msg.cmd, "join_game 42");
        
        // Servidor responde con GAME_JOINED
        GameJoinedResponse response;
        response.game_id = 42;
        response.player_id = 200;
        response.success = true;
        
        proto.sendMessage(response);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cliente se conecta y envía JOIN_GAME
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    
    ClientMessage join_msg;
    join_msg.cmd = "join_game 42";
    proto_client.sendMessage(join_msg);
    
    // Cliente espera respuesta
    GameJoinedResponse resp = proto_client.receiveGameJoined();
    
    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.game_id, 42);
    EXPECT_EQ(resp.player_id, 200);

    server_thread.join();
}

// ================================================================
// TEST: Servidor responde con error (success=false)
// ================================================================
TEST(LobbyProtocolTest, JoinGameFailure)
{
    std::thread server_thread([&]() {
        Socket acceptor(TEST_PORT);
        Socket peer = acceptor.accept();
        Protocol proto(std::move(peer));

        // Servidor espera y responde con error
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Servidor responde con error (partida no existe)
        GameJoinedResponse response;
        response.game_id = 999;
        response.player_id = 0;
        response.success = false;
        
        proto.sendMessage(response);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cliente intenta unirse a partida inexistente
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    
    ClientMessage join_msg;
    join_msg.cmd = "join_game 999";
    proto_client.sendMessage(join_msg);
    
    // Cliente espera respuesta de error
    GameJoinedResponse resp = proto_client.receiveGameJoined();
    
    EXPECT_FALSE(resp.success);
    EXPECT_EQ(resp.game_id, 999);
    EXPECT_EQ(resp.player_id, 0);

    server_thread.join();
}

// ================================================================
// TEST: Un cliente crea partida y otro se joinea
// ================================================================
TEST(LobbyProtocolTest, CreateAndJoinGame)
{
    const char* TEST_PORT_MULTI = "50301";
    uint32_t assigned_game_id = 0;
    
    std::thread server_thread([&]() {
        Socket acceptor(TEST_PORT_MULTI);
        
        // Primer cliente se conecta
        Socket peer1 = acceptor.accept();
        Protocol proto1(std::move(peer1));
        
        // Servidor recibe CREATE_GAME del primer cliente
        ClientMessage msg1 = proto1.receiveClientMessage();
        EXPECT_EQ(msg1.cmd, "create_game");
        
        // Servidor asigna un ID de partida (por ejemplo, 5)
        assigned_game_id = 5;
        GameJoinedResponse response1;
        response1.game_id = assigned_game_id;
        response1.player_id = 1;
        response1.success = true;
        proto1.sendMessage(response1);
        
        // Segundo cliente se conecta
        Socket peer2 = acceptor.accept();
        Protocol proto2(std::move(peer2));
        
        // Servidor recibe JOIN_GAME del segundo cliente
        ClientMessage msg2 = proto2.receiveClientMessage();
        EXPECT_EQ(msg2.cmd, "join_game 5");
        
        // Servidor responde permitiendo el join
        GameJoinedResponse response2;
        response2.game_id = assigned_game_id;
        response2.player_id = 2;
        response2.success = true;
        proto2.sendMessage(response2);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Primer cliente: crea la partida
    Socket client1("localhost", TEST_PORT_MULTI);
    Protocol proto_client1(std::move(client1));
    
    ClientMessage create_msg;
    create_msg.cmd = "create_game";
    proto_client1.sendMessage(create_msg);
    
    GameJoinedResponse resp1 = proto_client1.receiveGameJoined();
    EXPECT_TRUE(resp1.success);
    EXPECT_EQ(resp1.player_id, 1);
    uint32_t game_id = resp1.game_id;
    
    // Segundo cliente: se joinea a la partida creada
    Socket client2("localhost", TEST_PORT_MULTI);
    Protocol proto_client2(std::move(client2));
    
    ClientMessage join_msg;
    join_msg.cmd = "join_game " + std::to_string(game_id);
    proto_client2.sendMessage(join_msg);
    
    GameJoinedResponse resp2 = proto_client2.receiveGameJoined();
    EXPECT_TRUE(resp2.success);
    EXPECT_EQ(resp2.game_id, game_id);
    EXPECT_EQ(resp2.player_id, 2);

    server_thread.join();
}
