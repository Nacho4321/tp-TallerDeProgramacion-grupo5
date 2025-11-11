#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../client/game_client_handler.h"
#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/queue.h"
#include "../common/constants.h"
#include "../server/server_handler.h"
#include "../common/messages.h"

using namespace std; // Para los literales de chrono

static const char *TEST_PORT = "50200";

TEST(FullIntegrationTest, CompleteClientServerCommunication)
{
    // Levantamos el servidor con el nuevo ServerHandler en un hilo
    std::thread server_thread([&]() {
        ServerHandler server(TEST_PORT);
        server.start();
        // Esperamos a que el server reciba el mensaje
        ClientHandlerMessage in;
        bool got = false;
        for (int i = 0; i < 20 && !got; ++i) { // ~200ms timeout
            got = server.try_receive(in);
            if (!got) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(got);
    EXPECT_EQ(in.msg.cmd, MOVE_UP_PRESSED_STR);
    EXPECT_EQ(in.msg.player_id, -1);
    EXPECT_EQ(in.msg.game_id, -1);

        // Enviamos respuesta por broadcast con posiciones
        ServerMessage response_srv;
        PlayerPositionUpdate update;
        update.player_id = 1;
        update.new_pos.new_X = 100.0f;
        update.new_pos.new_Y = 200.0f;
        update.new_pos.direction_x = MovementDirectionX::right;
        update.new_pos.direction_y = MovementDirectionY::not_vertical;
        response_srv.positions.push_back(update);
        server.broadcast(response_srv);

        // El servidor espera que el cliente reciba
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        server.stop();
    });

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


    // Cliente espera respuesta (poll con timeout breve)
    ServerMessage response;
    bool got = false;
    for (int i = 0; i < 50 && !got; ++i) { // hasta ~500ms
        got = client.try_receive(response);
        if (!got) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(got);
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