#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/constants.h"

#include "../server/acceptor.h"
#include "../server/message_handler.h"
#include "../server/game_monitor.h"
#include "../common/messages.h"
#include "../common/queue.h"

static const char *TEST_PORT = "50100"; // usa un puerto distinto de los tests anteriores

// MessageHandler mock para tests que pushea a una cola
class TestMessageHandler : public MessageHandler {
private:
    Queue<ClientHandlerMessage> *test_inbox;
    std::vector<std::shared_ptr<Queue<ServerMessage>>> saved_outboxes;  // Para tests de broadcast
public:
    TestMessageHandler(GameMonitor &games_mon,
                    Queue<ClientHandlerMessage> *inbox_for_test = nullptr)
        : MessageHandler(games_mon), test_inbox(inbox_for_test) {}
    
    void handle_message(ClientHandlerMessage &message) override {
        if (test_inbox) {
            // Guardar outbox para tests de broadcast
            if (message.outbox) {
                saved_outboxes.push_back(message.outbox);
            }
            test_inbox->push(message);
        }
        // No llamar a MessageHandler::handle_message para los tests simples
    }
    
    std::shared_ptr<Queue<ServerMessage>> get_outbox(size_t index) {
        if (index < saved_outboxes.size()) {
            return saved_outboxes[index];
        }
        return nullptr;
    }
};

// ================================================================
// TEST: Cliente se conecta y envía mensaje al Acceptor
// ================================================================
TEST(AcceptorIntegrationTest, ClientConnectsAndSendsMessage)
{
    Queue<ClientHandlerMessage> inbox;
    GameMonitor games_monitor;
    TestMessageHandler message_handler(games_monitor, &inbox);

    std::thread server_thread([&]() {
        Acceptor acceptor(TEST_PORT, message_handler);
        acceptor.start();
        
        // Esperamos a que llegue el mensaje del cliente
        ClientHandlerMessage ch_msg;
        bool got = false;
        for (int i = 0; i < 30 && !got; ++i) {
            got = inbox.try_pop(ch_msg);
            if (!got)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(got);
        EXPECT_EQ(ch_msg.msg.cmd, MOVE_UP_PRESSED_STR);
        EXPECT_EQ(ch_msg.msg.player_id, -1);
        EXPECT_EQ(ch_msg.msg.game_id, -1);
        
        acceptor.stop();
        acceptor.join();
    });

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
    Queue<ClientHandlerMessage> inbox;
    GameMonitor games_monitor;
    TestMessageHandler message_handler(games_monitor, &inbox);

    std::thread server_thread2([&]() {
        Acceptor acceptor(TEST_PORT, message_handler);
        acceptor.start();
        
        // Esperamos a que llegue al menos un mensaje del cliente para tener su outbox
        ClientHandlerMessage first_msg;
        bool got = false;
        for (int i = 0; i < 50 && !got; ++i) {
            got = inbox.try_pop(first_msg);
            if (!got) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(got) << "No se recibió mensaje inicial del cliente";

        // Ahora enviamos un broadcast a todos los clientes
        ServerMessage msg;
        msg.opcode = UPDATE_POSITIONS;

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


        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto outbox = message_handler.get_outbox(0);  // Obtener desde TestMessageHandler  
        if (outbox) {
            outbox->push(msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        acceptor.stop();
        acceptor.join();
    });

    // Cliente
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Socket client("localhost", TEST_PORT);
    Protocol proto_client(std::move(client));
    
    // Enviar mensaje inicial para que el servidor guarde nuestro outbox
    ClientMessage init_msg;
    init_msg.cmd = MOVE_UP_PRESSED_STR;
    proto_client.sendMessage(init_msg);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

    server_thread2.join();
}

// ================================================================
// TEST: Multiples clientes reciben broadcast desde el Acceptor
// ================================================================
TEST(AcceptorIntegrationTest, BroadcastToMultipleClients)
{
    Queue<ClientHandlerMessage> inbox;
    GameMonitor games_monitor;
    TestMessageHandler message_handler(games_monitor, &inbox);

    std::thread server_thread3([&]() {
        Acceptor acceptor(TEST_PORT, message_handler);
        acceptor.start();
        
        // Esperar a recibir mensajes de ambos clientes para tener sus outboxes
        ClientHandlerMessage msg1, msg2;
        bool got1 = false, got2 = false;
        for (int i = 0; i < 50 && (!got1 || !got2); ++i) {
            if (!got1) got1 = inbox.try_pop(msg1);
            if (!got2) got2 = inbox.try_pop(msg2);
            if (!got1 || !got2) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(got1 && got2) << "No se recibieron mensajes de ambos clientes";

        ServerMessage msg2_broadcast;
        msg2_broadcast.opcode = UPDATE_POSITIONS;

        Position pos1b;
        pos1b.new_X = 50.0f;
        pos1b.new_Y = 75.0f;
        pos1b.direction_x = MovementDirectionX::left;
        pos1b.direction_y = MovementDirectionY::up;
        PlayerPositionUpdate update1b;
        update1b.player_id = 1;
        update1b.new_pos = pos1b;
        msg2_broadcast.positions.push_back(update1b);

        Position pos2b;
        pos2b.new_X = 150.0f;
        pos2b.new_Y = 175.0f;
        pos2b.direction_x = MovementDirectionX::right;
        pos2b.direction_y = MovementDirectionY::down;
        PlayerPositionUpdate update2b;
        update2b.player_id = 2;
        update2b.new_pos = pos2b;
        msg2_broadcast.positions.push_back(update2b);


        auto outbox0 = message_handler.get_outbox(0);  // Desde TestMessageHandler
        auto outbox1 = message_handler.get_outbox(1);  // Desde TestMessageHandler
        if (outbox0) {
            outbox0->push(msg2_broadcast);
        }
        if (outbox1) {
            outbox1->push(msg2_broadcast);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        acceptor.stop();
        acceptor.join();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Creamos dos clientes
    Socket c1("localhost", TEST_PORT);
    Socket c2("localhost", TEST_PORT);

    Protocol p1(std::move(c1));
    Protocol p2(std::move(c2));
    
    // Ambos clientes envían un mensaje inicial
    ClientMessage init1, init2;
    init1.cmd = MOVE_UP_PRESSED_STR;
    init2.cmd = MOVE_DOWN_PRESSED_STR;
    p1.sendMessage(init1);
    p2.sendMessage(init2);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

    server_thread3.join();
}