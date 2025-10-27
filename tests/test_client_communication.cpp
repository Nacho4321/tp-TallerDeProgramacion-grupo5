#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../common/game_client_receiver.h"
#include "../common/game_client_sender.h"
#include "../common/queue.h"
#include "../common/socket.h"
#include "../common/protocol.h"
#include "../common/constants.h"

// Helper para iniciar servidor y cliente
static const char* TEST_PORT = "50001";

TEST(ClientCommunicationTest, SendAndReceiveMessage) {
    // Crear colas para mensajes
    Queue<DecodedMessage> incoming;
    Queue<std::string> outgoing;

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

    // Crear sender y receiver
    GameClientSender sender(proto_client, outgoing);
    GameClientReceiver receiver(proto_client, incoming);

    // Iniciar los hilos
    sender.start();
    receiver.start();

    // Enviar un mensaje
    outgoing.push(MOVE_UP_PRESSED_STR);

    // Dar tiempo para que se procese
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Intentar recibir respuesta
    DecodedMessage response;
    ASSERT_TRUE(incoming.try_pop(response));
    ASSERT_EQ(response.cmd, MOVE_UP_PRESSED_STR);

    // Detener los hilos
    sender.stop();
    receiver.stop();
    sender.join();
    receiver.join();

    server_thread.join();
}