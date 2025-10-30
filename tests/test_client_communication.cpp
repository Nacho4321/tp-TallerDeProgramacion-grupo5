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
    ClientMessage cl_msg = proto_server.receiveClientMessage();
    ASSERT_EQ(cl_msg.cmd, MOVE_UP_PRESSED_STR);
    // Enviar una respuesta con al menos una posición (si enviamos vacío,
    // el GameClientReceiver lo interpreta como EOF y cierra)
    ServerMessage msg;
    PlayerPositionUpdate upd;
    upd.player_id = 0;
    upd.new_pos.new_X = 0.0f;
    upd.new_pos.new_Y = 0.0f;
    upd.new_pos.direction_x = MovementDirectionX::not_horizontal;
    upd.new_pos.direction_y = MovementDirectionY::not_vertical;
    msg.positions.push_back(upd);
    proto_server.sendMessage(msg);
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
    ServerMessage response;
    ASSERT_TRUE(handler.try_receive(response));
    // Esperamos 1 posición en la respuesta (la que envía el servidor en el test)
    ASSERT_EQ(response.positions.size(), 1);
    EXPECT_EQ(response.positions[0].player_id, 0);
    EXPECT_FLOAT_EQ(response.positions[0].new_pos.new_X, 0.0f);
    EXPECT_FLOAT_EQ(response.positions[0].new_pos.new_Y, 0.0f);


    // Detener y esperar al handler
    handler.stop();
    handler.join();

    server_thread.join();
}