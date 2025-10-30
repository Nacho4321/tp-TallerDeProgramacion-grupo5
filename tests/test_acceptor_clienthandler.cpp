#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/constants.h"

#include "../server/acceptor.h"
#include "../server/client_handler.h"
#include "../server/outbox_monitor.h"
#include "../common/messages.h"
#include "../common/queue.h"

static const char *TEST_PORT = "50100"; // usa un puerto distinto de los tests anteriores

// ================================================================
// TEST: Cliente se conecta y envía mensaje al Acceptor
// ================================================================
TEST(AcceptorIntegrationTest, ClientConnectsAndSendsMessage)
{
    Queue<ClientHandlerMessage> global_inbox;
    Queue<int> players;
    OutboxMonitor outbox_monitor;
    std::thread server_thread([&]()
                              {
        Socket listener(TEST_PORT);  // crea socket servidor
        Acceptor acceptor(listener, global_inbox, players, outbox_monitor);
        acceptor.start();

        // Esperamos a que llegue un mensaje desde el cliente
        ClientHandlerMessage ch_msg = global_inbox.pop();
        ClientMessage msg = ch_msg.msg;
        EXPECT_EQ(msg.cmd, MOVE_UP_PRESSED_STR);

        acceptor.stop();
        acceptor.join(); });

    // Dejamos tiempo para que el servidor se levante
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Cliente se conecta y envía mensaje
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    ClientMessage client_msg;
    client_msg.cmd = MOVE_UP_PRESSED_STR;
    proto_client.sendMessage(client_msg);

    server_thread.join();
}

// ================================================================
// TEST: Broadcast desde el Acceptor llega al cliente
// ================================================================
TEST(AcceptorIntegrationTest, BroadcastMessageToClient)
{
    Queue<ClientHandlerMessage> global_inbox;
    Queue<int> players;
    OutboxMonitor outbox_monitor;
    std::thread server_thread([&]()
                              {
        Socket listener(TEST_PORT);
        Acceptor acceptor(listener, global_inbox, players, outbox_monitor);
        acceptor.start();

        // Esperamos un poco a que se conecte el cliente
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Enviamos un broadcast a todos los clientes
        ServerMessage msg;

        Position pos1;
        pos1.new_X = 50.0f;
        pos1.new_Y = 75.0f;
        pos1.direction_x = MovementDirectionX::left;
        pos1.direction_y = MovementDirectionY::up;
        PlayerPositionUpdate update1;
        update1.player_id = 1;
        update1.new_pos = pos1;
        msg.positions.push_back(update1);

        Position pos2;
        pos2.new_X = 150.0f;
        pos2.new_Y = 175.0f;
        pos2.direction_x = MovementDirectionX::right;
        pos2.direction_y = MovementDirectionY::down;
        PlayerPositionUpdate update2;
        update2.player_id = 2;
        update2.new_pos = pos2;
        msg.positions.push_back(update2);

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
    ServerMessage received = proto_client.receiveServerMessage();
    EXPECT_EQ(received.positions.size(), 2);
    EXPECT_EQ(received.positions[0].player_id, 1);
    EXPECT_FLOAT_EQ(received.positions[0].new_pos.new_X, 50.0f);
    EXPECT_FLOAT_EQ(received.positions[0].new_pos.new_Y, 75.0f);
    EXPECT_EQ(received.positions[0].new_pos.direction_x, MovementDirectionX::left);
    EXPECT_EQ(received.positions[0].new_pos.direction_y, MovementDirectionY::up);
    EXPECT_EQ(received.positions[1].player_id, 2);
    EXPECT_FLOAT_EQ(received.positions[1].new_pos.new_X, 150.0f);
    EXPECT_FLOAT_EQ(received.positions[1].new_pos.new_Y, 175.0f);
    EXPECT_EQ(received.positions[1].new_pos.direction_x, MovementDirectionX::right);
    EXPECT_EQ(received.positions[1].new_pos.direction_y, MovementDirectionY::down);

    server_thread.join();
}

// ================================================================
// TEST: Multiples clientes reciben broadcast desde el Acceptor
// ================================================================
TEST(AcceptorIntegrationTest, BroadcastToMultipleClients)
{
    Queue<ClientHandlerMessage> global_inbox;
    Queue<int> players;
    OutboxMonitor outbox_monitor;
    Socket listener(TEST_PORT);
    Acceptor acceptor(listener, global_inbox, players, outbox_monitor);

    std::thread server_thread([&]()
                              {
        acceptor.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        ServerMessage msg;

        Position pos1;
        pos1.new_X = 50.0f;
        pos1.new_Y = 75.0f;
        pos1.direction_x = MovementDirectionX::left;
        pos1.direction_y = MovementDirectionY::up;
        PlayerPositionUpdate update1;
        update1.player_id = 1;
        update1.new_pos = pos1;
        msg.positions.push_back(update1);

        Position pos2;
        pos2.new_X = 150.0f;
        pos2.new_Y = 175.0f;
        pos2.direction_x = MovementDirectionX::right;
        pos2.direction_y = MovementDirectionY::down;
        PlayerPositionUpdate update2;
        update2.player_id = 2;
        update2.new_pos = pos2;
        msg.positions.push_back(update2);

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

    auto m1 = p1.receiveServerMessage();
    auto m2 = p2.receiveServerMessage();

    EXPECT_EQ(m1.positions.size(), 2);
    EXPECT_EQ(m2.positions.size(), 2);
    EXPECT_EQ(m1.positions[0].player_id, 1);
    EXPECT_EQ(m2.positions[0].player_id, 1);
    EXPECT_FLOAT_EQ(m1.positions[0].new_pos.new_X, 50.0f);
    EXPECT_FLOAT_EQ(m2.positions[0].new_pos.new_X, 50.0f);
    EXPECT_FLOAT_EQ(m1.positions[0].new_pos.new_Y, 75.0f);
    EXPECT_FLOAT_EQ(m2.positions[0].new_pos.new_Y, 75.0f);
    EXPECT_EQ(m1.positions[0].new_pos.direction_x, MovementDirectionX::left);
    EXPECT_EQ(m2.positions[0].new_pos.direction_x, MovementDirectionX::left);
    EXPECT_EQ(m1.positions[0].new_pos.direction_y, MovementDirectionY::up);
    EXPECT_EQ(m2.positions[0].new_pos.direction_y, MovementDirectionY::up);
    EXPECT_EQ(m1.positions[1].player_id, 2);
    EXPECT_EQ(m2.positions[1].player_id, 2);
    EXPECT_FLOAT_EQ(m1.positions[1].new_pos.new_X, 150.0f);
    EXPECT_FLOAT_EQ(m2.positions[1].new_pos.new_X, 150.0f);
    EXPECT_FLOAT_EQ(m1.positions[1].new_pos.new_Y, 175.0f);
    EXPECT_FLOAT_EQ(m2.positions[1].new_pos.new_Y, 175.0f);
    EXPECT_EQ(m1.positions[1].new_pos.direction_x, MovementDirectionX::right);
    EXPECT_EQ(m2.positions[1].new_pos.direction_x, MovementDirectionX::right);
    EXPECT_EQ(m1.positions[1].new_pos.direction_y, MovementDirectionY::down);
    EXPECT_EQ(m2.positions[1].new_pos.direction_y, MovementDirectionY::down);

    server_thread.join();
}
