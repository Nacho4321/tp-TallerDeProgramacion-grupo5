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
    ServerMessage msg;
    msg.opcode = GAME_JOINED;
    msg.game_id = response.game_id;
    msg.player_id = response.player_id;
    msg.success = response.success;
    proto.sendMessage(msg);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cliente se conecta y envía CREATE_GAME
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    
    ClientMessage create_msg;
    create_msg.cmd = "create_game";
    proto_client.sendMessage(create_msg);
    
    // Cliente espera respuesta
    ServerMessage outServer;
    GameJoinedResponse outJoined;
    uint8_t outOpcode;
    bool ok = proto_client.receiveAnyServerPacket(outServer, outJoined, outOpcode);
    EXPECT_TRUE(ok);
    EXPECT_EQ(outOpcode, GAME_JOINED);
    EXPECT_TRUE(outJoined.success);
    EXPECT_EQ(outJoined.game_id, 1);
    EXPECT_EQ(outJoined.player_id, 100);

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
    EXPECT_EQ(msg.cmd, "join_game");
    EXPECT_EQ(msg.game_id, 42);
        
        // Servidor responde con GAME_JOINED
    GameJoinedResponse response;
    response.game_id = 42;
    response.player_id = 200;
    response.success = true;
    ServerMessage serverMsg;
    serverMsg.opcode = GAME_JOINED;
    serverMsg.game_id = response.game_id;
    serverMsg.player_id = response.player_id;
    serverMsg.success = response.success;
    proto.sendMessage(serverMsg);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cliente se conecta y envía JOIN_GAME
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    
    ClientMessage join_msg;
    join_msg.cmd = "join_game";
    join_msg.game_id = 42;
    proto_client.sendMessage(join_msg);
    
    // Cliente espera respuesta
    ServerMessage outServer;
    GameJoinedResponse outJoined;
    uint8_t outOpcode;
    bool ok = proto_client.receiveAnyServerPacket(outServer, outJoined, outOpcode);
    EXPECT_TRUE(ok);
    EXPECT_EQ(outOpcode, GAME_JOINED);
    EXPECT_TRUE(outJoined.success);
    EXPECT_EQ(outJoined.game_id, 42);
    EXPECT_EQ(outJoined.player_id, 200);

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
    ServerMessage serverMsg;
    serverMsg.opcode = GAME_JOINED;
    serverMsg.game_id = response.game_id;
    serverMsg.player_id = response.player_id;
    serverMsg.success = response.success;
    proto.sendMessage(serverMsg);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cliente intenta unirse a partida inexistente
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    
    ClientMessage join_msg;
    join_msg.cmd = "join_game 999";
    proto_client.sendMessage(join_msg);
    
    // Cliente espera respuesta de error
    ServerMessage outServer;
    GameJoinedResponse outJoined;
    uint8_t outOpcode;
    bool ok = proto_client.receiveAnyServerPacket(outServer, outJoined, outOpcode);
    EXPECT_TRUE(ok);
    EXPECT_EQ(outOpcode, GAME_JOINED);
    EXPECT_FALSE(outJoined.success);
    EXPECT_EQ(outJoined.game_id, 999);
    EXPECT_EQ(outJoined.player_id, 0);

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
    ServerMessage serverMsg1;
    serverMsg1.opcode = GAME_JOINED;
    serverMsg1.game_id = response1.game_id;
    serverMsg1.player_id = response1.player_id;
    serverMsg1.success = response1.success;
    proto1.sendMessage(serverMsg1);
        
        // Segundo cliente se conecta
        Socket peer2 = acceptor.accept();
        Protocol proto2(std::move(peer2));
        
    // Servidor recibe JOIN_GAME del segundo cliente
    ClientMessage msg2 = proto2.receiveClientMessage();
    EXPECT_EQ(msg2.cmd, "join_game");
    EXPECT_EQ(msg2.game_id, 5);
        
        // Servidor responde permitiendo el join
    GameJoinedResponse response2;
    response2.game_id = assigned_game_id;
    response2.player_id = 2;
    response2.success = true;
    ServerMessage serverMsg2;
    serverMsg2.opcode = GAME_JOINED;
    serverMsg2.game_id = response2.game_id;
    serverMsg2.player_id = response2.player_id;
    serverMsg2.success = response2.success;
    proto2.sendMessage(serverMsg2);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Primer cliente: crea la partida
    Socket client1("localhost", TEST_PORT_MULTI);
    Protocol proto_client1(std::move(client1));
    
    ClientMessage create_msg;
    create_msg.cmd = "create_game";
    proto_client1.sendMessage(create_msg);
    
    ServerMessage outServer1;
    GameJoinedResponse outJoined1;
    uint8_t outOpcode1;
    bool ok1 = proto_client1.receiveAnyServerPacket(outServer1, outJoined1, outOpcode1);
    EXPECT_TRUE(ok1);
    EXPECT_EQ(outOpcode1, GAME_JOINED);
    EXPECT_TRUE(outJoined1.success);
    EXPECT_EQ(outJoined1.player_id, 1);
    uint32_t game_id = outJoined1.game_id;

    // Segundo cliente: se joinea a la partida creada
    Socket client2("localhost", TEST_PORT_MULTI);
    Protocol proto_client2(std::move(client2));

    ClientMessage join_msg;
    join_msg.cmd = "join_game";
    join_msg.game_id = static_cast<int32_t>(game_id);
    proto_client2.sendMessage(join_msg);

    ServerMessage outServer2;
    GameJoinedResponse outJoined2;
    uint8_t outOpcode2;
    bool ok2 = proto_client2.receiveAnyServerPacket(outServer2, outJoined2, outOpcode2);
    EXPECT_TRUE(ok2);
    EXPECT_EQ(outOpcode2, GAME_JOINED);
    EXPECT_TRUE(outJoined2.success);
    EXPECT_EQ(outJoined2.game_id, game_id);
    EXPECT_EQ(outJoined2.player_id, 2);

    server_thread.join();
}
