#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/constants.h"

#include "../server/acceptor.h"
#include "../server/client_handler.h"
#include "../server/outbox_monitor.h"
#include "../server/messages.h"
#include "../common/queue.h"

static const char *TEST_PORT = "50100"; // usa un puerto distinto de los tests anteriores

// ================================================================
// TEST: Cliente se conecta y envía mensaje al Acceptor
// ================================================================
TEST(AcceptorIntegrationTest, ClientConnectsAndSendsMessage)
{
    Queue<IncomingMessage> global_inbox;

    std::thread server_thread([&]()
                              {
        Socket listener(TEST_PORT);  // crea socket servidor
        Acceptor acceptor(listener, global_inbox);
        acceptor.start();

        // Esperamos a que llegue un mensaje desde el cliente
        IncomingMessage msg = global_inbox.pop();
        EXPECT_EQ(msg.cmd, MOVE_UP_PRESSED_STR);

        acceptor.stop();
        acceptor.join(); });

    // Dejamos tiempo para que el servidor se levante
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Cliente se conecta y envía mensaje
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    proto_client.sendMessage(MOVE_UP_PRESSED_STR);

    server_thread.join();
}

// ================================================================
// TEST: Broadcast desde el Acceptor llega al cliente
// ================================================================
TEST(AcceptorIntegrationTest, BroadcastMessageToClient)
{
    Queue<IncomingMessage> global_inbox;

    std::thread server_thread([&]()
                              {
        Socket listener(TEST_PORT);
        Acceptor acceptor(listener, global_inbox);
        acceptor.start();

        // Esperamos un poco a que se conecte el cliente
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Enviamos un broadcast a todos los clientes
        OutgoingMessage msg;
        msg.cmd = UPDATE_POSITIONS_STR;
        acceptor.broadcast(msg);

        // Esperamos a que el cliente reciba
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        acceptor.stop();
        acceptor.join(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cliente
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));

    // Recibimos el mensaje broadcast
    DecodedMessage received = proto_client.receiveMessage();
    EXPECT_EQ(received.cmd, UPDATE_POSITIONS_STR);

    server_thread.join();
}

// ================================================================
// TEST: Multiples clientes reciben broadcast desde el Acceptor
// ================================================================
TEST(AcceptorIntegrationTest, BroadcastToMultipleClients)
{
    Queue<IncomingMessage> global_inbox;
    Socket listener(TEST_PORT);
    Acceptor acceptor(listener, global_inbox);

    std::thread server_thread([&]()
                              {
        acceptor.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        OutgoingMessage msg;
        msg.cmd = UPDATE_POSITIONS_STR;
        acceptor.broadcast(msg);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        acceptor.stop();
        acceptor.join(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Creamos dos clientes
    Socket c1("localhost", TEST_PORT);
    Socket c2("localhost", TEST_PORT);

    Protocol p1(std::move(c1));
    Protocol p2(std::move(c2));

    auto m1 = p1.receiveMessage();
    auto m2 = p2.receiveMessage();

    EXPECT_EQ(m1.cmd, UPDATE_POSITIONS_STR);
    EXPECT_EQ(m2.cmd, UPDATE_POSITIONS_STR);

    server_thread.join();
}
