#include "client_receiver.h"

ClientReceiver::ClientReceiver(Protocol &proto, int id, Queue<ClientHandlerMessage> &global_inbox) : protocol(proto), client_id(id), global_inbox(global_inbox) {}

void ClientReceiver::run()
{
    try
    {
        std::cout << "[ClientReceiver(Server)] Hilo receiver iniciado para cliente " << client_id << std::endl;
        while (should_keep_running())
        {
            ClientMessage client_msg = protocol.receiveClientMessage();
            if (client_msg.cmd.empty())
            {
                ClientHandlerMessage leave_msg;
                leave_msg.client_id = client_id;
                leave_msg.msg.cmd = LEAVE_GAME_STR; 
                leave_msg.msg.player_id = -1;
                leave_msg.msg.game_id = -1;
                try {
                    global_inbox.push(leave_msg);
                } catch (...) {
                    // Si falla la push por cola cerrada u otro error, no podemos hacer más.
                }
                break;
            }

            ClientHandlerMessage msg; // para agregarle el id
            msg.client_id = client_id;
            msg.msg = client_msg;
            global_inbox.push(msg);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Receiver] Exception: " << e.what() << std::endl;
    }
    std::cout << "[ClientReceiver(Server)] Terminando receiver de cliente " << client_id << std::endl;
}