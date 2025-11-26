#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/constants.h"

#include "../server/acceptor.h"
#include "../server/outbox_monitor.h"
#include "../server/message_handler.h"
#include "../server/game_monitor.h"
#include "../common/messages.h"
#include "../common/queue.h"

static const char *TEST_PORT = "50100"; // usa un puerto distinto de los tests anteriores

// MessageHandler mock para tests que pushea a una cola
class TestMessageHandler : public MessageHandler {
private:
    Queue<ClientHandlerMessage> *test_inbox;
public:
    TestMessageHandler(std::unordered_map<int, std::shared_ptr<Queue<Event>>> &game_qs,
                    std::mutex &game_qs_mutex,
                    GameMonitor &games_mon,
                    OutboxMonitor &outbox,
                    Queue<ClientHandlerMessage> *inbox_for_test = nullptr)
        : MessageHandler(game_qs, game_qs_mutex, games_mon, outbox), test_inbox(inbox_for_test) {}
    
    void handle_message(ClientHandlerMessage &message) override {
        if (test_inbox) {
            test_inbox->push(message);
        }
        // No llamar a MessageHandler::handle_message para los tests simples
    }
};

// ================================================================
// TEST: Cliente se conecta y envía mensaje al Acceptor
// ================================================================
TEST(AcceptorIntegrationTest, ClientConnectsAndSendsMessage)
{
    Queue<ClientHandlerMessage> inbox;
    OutboxMonitor outboxes;
    std::unordered_map<int, std::shared_ptr<Queue<Event>>> game_queues;
    std::mutex game_queues_mutex;
    GameMonitor games_monitor(game_queues, game_queues_mutex);
    TestMessageHandler message_handler(game_queues, game_queues_mutex, games_monitor, outboxes, &inbox);

    std::thread server_thread([&]() {
        Acceptor acceptor(TEST_PORT, message_handler, outboxes);
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
    OutboxMonitor outboxes;
    std::unordered_map<int, std::shared_ptr<Queue<Event>>> game_queues;
    std::mutex game_queues_mutex;
    GameMonitor games_monitor(game_queues, game_queues_mutex);
    TestMessageHandler message_handler(game_queues, game_queues_mutex, games_monitor, outboxes, &inbox);

    std::thread server_thread2([&]() {
        Acceptor acceptor(TEST_PORT, message_handler, outboxes);
        acceptor.start();
        
        // Esperamos un poco a que se conecte el cliente
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Enviamos un broadcast a todos los clientes
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
        auto outbox = outboxes.get(0);  
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
    OutboxMonitor outboxes;
    std::unordered_map<int, std::shared_ptr<Queue<Event>>> game_queues;
    std::mutex game_queues_mutex;
    GameMonitor games_monitor(game_queues, game_queues_mutex);
    TestMessageHandler message_handler(game_queues, game_queues_mutex, games_monitor, outboxes, &inbox);

    std::thread server_thread3([&]() {
        Acceptor acceptor(TEST_PORT, message_handler, outboxes);
        acceptor.start();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        ServerMessage msg2;
        msg2.opcode = UPDATE_POSITIONS;

        Position pos1b;
        pos1b.new_X = 50.0f;
        pos1b.new_Y = 75.0f;
        pos1b.direction_x = MovementDirectionX::left;
        pos1b.direction_y = MovementDirectionY::up;
        PlayerPositionUpdate update1b;
        update1b.player_id = 1;
        update1b.new_pos = pos1b;
        msg2.positions.push_back(update1b);

        Position pos2b;
        pos2b.new_X = 150.0f;
        pos2b.new_Y = 175.0f;
        pos2b.direction_x = MovementDirectionX::right;
        pos2b.direction_y = MovementDirectionY::down;
        PlayerPositionUpdate update2b;
        update2b.player_id = 2;
        update2b.new_pos = pos2b;
        msg2.positions.push_back(update2b);


        auto outbox0 = outboxes.get(0);  
        auto outbox1 = outboxes.get(1);  
        if (outbox0) {
            outbox0->push(msg2);
        }
        if (outbox1) {
            outbox1->push(msg2);
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