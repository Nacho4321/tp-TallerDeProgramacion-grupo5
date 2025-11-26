#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../client/game_client_handler.h"
#include "../common/protocol.h"
#include "../common/socket.h"
#include "../common/queue.h"
#include "../common/constants.h"
#include "../server/acceptor.h"
#include "../server/message_handler.h"
#include "../server/game_monitor.h"
#include "../common/messages.h"

using namespace std;

static const char *TEST_PORT = "50200";

// MessageHandler mock para tests
class TestMessageHandler : public MessageHandler {
private:
    Queue<ClientHandlerMessage> *test_inbox;
    std::vector<std::shared_ptr<Queue<ServerMessage>>> saved_outboxes;
public:
    TestMessageHandler(GameMonitor &games_mon,
                    Queue<ClientHandlerMessage> *inbox_for_test = nullptr)
        : MessageHandler(games_mon), test_inbox(inbox_for_test) {}
    
    void handle_message(ClientHandlerMessage &message) override {
        if (test_inbox) {
            if (message.outbox) {
                saved_outboxes.push_back(message.outbox);
            }
            test_inbox->push(message);
        }
    }
    
    std::shared_ptr<Queue<ServerMessage>> get_outbox(size_t index) {
        if (index < saved_outboxes.size()) {
            return saved_outboxes[index];
        }
        return nullptr;
    }
};

TEST(FullIntegrationTest, CompleteClientServerCommunication)
{
    // Creamos las estructuras necesarias para el Acceptor
    Queue<ClientHandlerMessage> inbox;
    GameMonitor games_monitor;
    TestMessageHandler message_handler(games_monitor, &inbox);

    // Levantamos el servidor con Acceptor en un hilo
    std::thread server_thread([&]() {
        Acceptor acceptor(TEST_PORT, message_handler);
        acceptor.start();

        // Esperamos a que el server reciba el mensaje del cliente
        ClientHandlerMessage in;
        bool got = false;
        for (int i = 0; i < 20 && !got; ++i) { // ~200ms timeout
            got = inbox.try_pop(in);
            if (!got) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(got);
        EXPECT_EQ(in.msg.cmd, MOVE_UP_PRESSED_STR);
        EXPECT_EQ(in.msg.player_id, -1);
        EXPECT_EQ(in.msg.game_id, -1);

        // Enviamos respuesta por broadcast con posiciones
        ServerMessage response_srv;
        response_srv.opcode = UPDATE_POSITIONS;
        PlayerPositionUpdate update;
        update.player_id = 1;
        update.new_pos.new_X = 100.0f;
        update.new_pos.new_Y = 200.0f;
        update.new_pos.direction_x = MovementDirectionX::right;
        update.new_pos.direction_y = MovementDirectionY::not_vertical;
        response_srv.positions.push_back(update);
        
      
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto outbox = message_handler.get_outbox(0);  // Desde TestMessageHandler
        if (outbox) {
            outbox->push(response_srv);
        }

        // El servidor espera que el cliente reciba
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        acceptor.stop();
        acceptor.join();
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