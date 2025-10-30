#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../client/game_client_handler.h"
#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/queue.h"
#include "../common/constants.h"
#include "../server/acceptor.h"
#include "../common/messages.h"

using namespace std; // Para los literales de chrono

static const char *TEST_PORT = "50200";

TEST(FullIntegrationTest, CompleteClientServerCommunication)
{
    Queue<ClientHandlerMessage> global_inbox;
    Queue<int> players;
    OutboxMonitor outbox_monitor;
    // Thread para el servidor con Acceptor
    std::thread server_thread([&]()
                              {
        Socket listener(TEST_PORT);
        Acceptor acceptor(listener, global_inbox,players, outbox_monitor);
        acceptor.start();

        // Esperamos mensaje del cliente
        ClientHandlerMessage msg = global_inbox.pop();
        EXPECT_EQ(msg.msg.cmd, MOVE_UP_PRESSED_STR);

        // Enviamos respuesta por broadcast con posiciones
        ServerMessage response;
        PlayerPositionUpdate update;
        update.player_id = 1;
        update.new_pos.new_X = 100.0f;
        update.new_pos.new_Y = 200.0f;
        update.new_pos.direction_x = MovementDirectionX::right;
        update.new_pos.direction_y = MovementDirectionY::not_vertical;
        response.positions.push_back(update);
        acceptor.broadcast(response);

        // El servidor espera que el cliente reciba
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        acceptor.stop();
        acceptor.join(); });

    // Esperamos a que el servidor esté listo
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Cliente se conecta y crea su handler
    Socket peer("localhost", TEST_PORT);
    Protocol proto(std::move(peer));
    GameClientHandler client(proto);

    // Iniciamos el cliente
    client.start();

    // Cliente envía mensaje
    client.send(MOVE_UP_PRESSED_STR);

    // Esperamos un poco para que el mensaje llegue
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cliente espera respuesta
    ServerMessage response;
    ASSERT_TRUE(client.try_receive(response));
    EXPECT_EQ(response.positions.size(), 1);
    EXPECT_EQ(response.positions[0].player_id, 1);
    EXPECT_FLOAT_EQ(response.positions[0].new_pos.new_X, 100.0f);
    EXPECT_FLOAT_EQ(response.positions[0].new_pos.new_Y, 200.0f);
    EXPECT_EQ(response.positions[0].new_pos.direction_x, MovementDirectionX::right);
    EXPECT_EQ(response.positions[0].new_pos.direction_y, MovementDirectionY::not_vertical);

    client.stop();
    client.join();
    server_thread.join();
}