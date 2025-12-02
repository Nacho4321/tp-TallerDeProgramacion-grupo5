#include "client_receiver.h"
#include "lobby_handler.h"

ClientReceiver::ClientReceiver(Protocol &proto, int id, LobbyHandler &msg_admin, std::shared_ptr<Queue<ServerMessage>> out) 
    : protocol(proto), client_id(id), message_handler(msg_admin), outbox(out) {}

void ClientReceiver::run()
{
    try
    {
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
                leave_msg.outbox = outbox;  // Pasar outbox
                
                // Procesar directamente el mensaje de desconexión
                message_handler.handle_message(leave_msg);
                break;
            }

            ClientHandlerMessage msg;
            msg.client_id = client_id;
            msg.msg = client_msg;
            msg.outbox = outbox;  // Pasar outbox
            
            // Procesar mensaje directamente en lugar de pushear a cola
            message_handler.handle_message(msg);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Receiver] Exception: " << e.what() << std::endl;
    }
}