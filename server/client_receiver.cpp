#include "client_receiver.h"
#include "message_handler.h"

ClientReceiver::ClientReceiver(Protocol &proto, int id, MessageHandler &msg_admin) 
    : protocol(proto), client_id(id), message_handler(msg_admin) {}

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
                
                // Procesar directamente el mensaje de desconexión
                message_handler.handle_message(leave_msg);
                break;
            }

            ClientHandlerMessage msg;
            msg.client_id = client_id;
            msg.msg = client_msg;
            
            // Procesar mensaje directamente en lugar de pushear a cola
            message_handler.handle_message(msg);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Receiver] Exception: " << e.what() << std::endl;
    }
    std::cout << "[ClientReceiver(Server)] Terminando receiver de cliente " << client_id << std::endl;
}