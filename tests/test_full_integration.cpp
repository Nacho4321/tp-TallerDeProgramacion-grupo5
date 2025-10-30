#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../client/game_client_handler.h"
#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/queue.h"
#include "../common/constants.h"
#include "../server/acceptor.h"
#include "../server/messages.h"

using namespace std; // Para los literales de chrono

static const char *TEST_PORT = "50200";

TEST(FullIntegrationTest, CompleteClientServerCommunication)
{
    Queue<IncomingMessage> global_inbox;
    Queue<int> players;
    // Thread para el servidor con Acceptor
    std::thread server_thread([&]()
                              {
        Socket listener(TEST_PORT);
        Acceptor acceptor(listener, global_inbox,players);
        acceptor.start();

        // Esperamos mensaje del cliente
        IncomingMessage msg = global_inbox.pop();
        EXPECT_EQ(msg.cmd, MOVE_UP_PRESSED_STR);

        // Enviamos respuesta por broadcast
        OutgoingMessage response;
        response.cmd = MOVE_DOWN_PRESSED_STR;
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
    DecodedMessage response;
    ASSERT_TRUE(client.try_receive(response));
    EXPECT_EQ(response.cmd, MOVE_DOWN_PRESSED_STR);

    client.stop();
    client.join();
    server_thread.join();
}