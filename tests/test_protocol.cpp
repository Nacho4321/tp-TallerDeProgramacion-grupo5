#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/constants.h"

// Helper para iniciar servidor y cliente en puertos distintos
static const char* TEST_PORT = "50000";

// =====================================================================
// TESTS
// =====================================================================

TEST(ProtocolLocalhostTest, SendAndReceiveUpPressed) {
    std::thread server_thread([]() {
        Socket listener(TEST_PORT);              // servidor escucha
        Socket peer = listener.accept();         // bloqueante
        Protocol proto_server(std::move(peer));  // protocolo del servidor
        ClientMessage msg = proto_server.receiveClientMessage(); // bloqueante
        EXPECT_EQ(msg.cmd, MOVE_UP_PRESSED_STR);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // da tiempo al server

    Socket client("localhost", TEST_PORT);       // cliente conecta
    Protocol proto_client(std::move(client));    // protocolo del cliente
    ClientMessage client_msg;
    client_msg.cmd = MOVE_UP_PRESSED_STR;
    proto_client.sendMessage(client_msg);

    server_thread.join();
}

TEST(ProtocolLocalhostTest, SendAndReceiveRightReleased) {
    std::thread server_thread([]() {
        Socket listener(TEST_PORT);
        Socket peer = listener.accept();
        Protocol proto_server(std::move(peer));
        ClientMessage msg = proto_server.receiveClientMessage();
        EXPECT_EQ(msg.cmd, MOVE_RIGHT_RELEASED_STR);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    ClientMessage client_msg;
    client_msg.cmd = MOVE_RIGHT_RELEASED_STR;
    proto_client.sendMessage(client_msg);

    server_thread.join();
}

TEST(ProtocolLocalhostTest, MultipleSequentialMessages) {
    std::thread server_thread([]() {
        Socket listener(TEST_PORT);
        Socket peer = listener.accept();
        Protocol proto_server(std::move(peer));

        auto m1 = proto_server.receiveClientMessage();
        EXPECT_EQ(m1.cmd, MOVE_DOWN_PRESSED_STR);

        auto m2 = proto_server.receiveClientMessage();
        EXPECT_EQ(m2.cmd, MOVE_LEFT_RELEASED_STR);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));

    ClientMessage client_msg1;
    client_msg1.cmd = MOVE_DOWN_PRESSED_STR;
    proto_client.sendMessage(client_msg1);
    ClientMessage client_msg2;
    client_msg2.cmd = MOVE_LEFT_RELEASED_STR;
    proto_client.sendMessage(client_msg2);

    server_thread.join();
}
