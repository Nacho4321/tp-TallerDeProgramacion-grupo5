#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <string>
#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/constants.h"
#include "../common/messages.h"

// Puerto distinto para evitar colisiones con otros tests
static const char* TEST_PORT_GAMES = "50310";

// Helper: servidor simulado para create_game con nombre y luego get_games
TEST(GamesListingTest, CreateGameWithNameAndList) {
    std::thread server_thread([]() {
        Socket acceptor(TEST_PORT_GAMES);
        // Aceptar cliente que crea partida
        Socket peer1 = acceptor.accept();
        Protocol proto1(std::move(peer1));
        ClientMessage createReq = proto1.receiveClientMessage();
        // Debe ser create_game y tener nombre
        EXPECT_EQ(createReq.cmd, CREATE_GAME_STR);
        EXPECT_EQ(createReq.game_name, "SalaAlpha");
        // Responder GAME_JOINED
        GameJoinedResponse joinResp1; joinResp1.game_id = 1; joinResp1.player_id = 10; joinResp1.success = true;
        proto1.sendMessage(joinResp1);
        // Aceptar cliente que pide listado
        Socket peer2 = acceptor.accept();
        Protocol proto2(std::move(peer2));
        ClientMessage listReq = proto2.receiveClientMessage();
        EXPECT_EQ(listReq.cmd, GET_GAMES_STR);
        // Responder listado con una partida
        ServerMessage listMsg; listMsg.opcode = GAMES_LIST;
        ServerMessage::GameSummary summary; summary.game_id = 1; summary.name = "SalaAlpha"; summary.player_count = 1;
        listMsg.games.push_back(summary);
        proto2.sendMessage(listMsg);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cliente 1 crea partida con nombre
    Socket client1("localhost", TEST_PORT_GAMES);
    Protocol proto_client1(std::move(client1));
    ClientMessage createMsg; createMsg.cmd = CREATE_GAME_STR; createMsg.game_name = "SalaAlpha"; // set nombre explícito
    proto_client1.sendMessage(createMsg);
    GameJoinedResponse r1 = proto_client1.receiveGameJoined();
    EXPECT_TRUE(r1.success);
    EXPECT_EQ(r1.game_id, 1);
    EXPECT_EQ(r1.player_id, 10);

    // Cliente 2 pide listado
    Socket client2("localhost", TEST_PORT_GAMES);
    Protocol proto_client2(std::move(client2));
    ClientMessage getMsg; getMsg.cmd = GET_GAMES_STR; // no extra payload
    proto_client2.sendMessage(getMsg);
    // Recibir server packet genérico
    ServerMessage serverList = proto_client2.receiveServerMessage();
    EXPECT_EQ(serverList.opcode, GAMES_LIST);
    ASSERT_EQ(serverList.games.size(), 1u);
    EXPECT_EQ(serverList.games[0].game_id, 1u);
    EXPECT_EQ(serverList.games[0].name, "SalaAlpha");
    EXPECT_EQ(serverList.games[0].player_count, 1u);

    server_thread.join();
}

// Caso: nombre vacío => servidor podría asignar nombre por defecto (probamos solo flujo encode sin nombre)
TEST(GamesListingTest, CreateGameWithoutNameEncodesEmpty) {
    std::thread server_thread([]() {
        Socket acceptor(TEST_PORT_GAMES);
        Socket peer = acceptor.accept();
        Protocol proto(std::move(peer));
        ClientMessage req = proto.receiveClientMessage();
        EXPECT_EQ(req.cmd, CREATE_GAME_STR);
        EXPECT_TRUE(req.game_name.empty());
        GameJoinedResponse jr; jr.game_id = 2; jr.player_id = 11; jr.success = true; proto.sendMessage(jr);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Socket client("localhost", TEST_PORT_GAMES);
    Protocol proto_client(std::move(client));
    ClientMessage createMsg; createMsg.cmd = CREATE_GAME_STR; // sin nombre
    proto_client.sendMessage(createMsg);
    GameJoinedResponse r = proto_client.receiveGameJoined();
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.game_id, 2);
    EXPECT_EQ(r.player_id, 11);
    server_thread.join();
}
