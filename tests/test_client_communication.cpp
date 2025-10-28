#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../client/game_client_handler.h"
#include "../common/socket.h"
#include "../common/protocol.h"
#include "../common/constants.h"

// Helper para iniciar servidor y cliente
static const char* TEST_PORT = "50001";

TEST(ClientCommunicationTest, SendAndReceiveMessage) {
    // Nada: el handler encapsula las colas internas

    std::thread server_thread([&]() {
        Socket listener(TEST_PORT);              // servidor escucha
        Socket peer = listener.accept();         // bloqueante
        Protocol proto_server(std::move(peer));  // protocolo del servidor

        // Recibir mensaje y devolverlo
        DecodedMessage msg = proto_server.receiveMessage();
        proto_server.sendMessage(msg.cmd);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // da tiempo al server

    // Crear conexión del cliente
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));

    // Crear un handler que maneja sender/receiver internamente
    GameClientHandler handler(proto_client);
    handler.start();

    // Enviar un mensaje usando la interfaz del handler
    handler.send(MOVE_UP_PRESSED_STR);

    // Dar tiempo para que se procese
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Intentar recibir respuesta
    DecodedMessage response;
    ASSERT_TRUE(handler.try_receive(response));
    ASSERT_EQ(response.cmd, MOVE_UP_PRESSED_STR);

    // Detener y esperar al handler
    handler.stop();
    handler.join();

    server_thread.join();
}