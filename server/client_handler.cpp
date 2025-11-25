#include "client_handler.h"

// ---------------- ClientReceiver ----------------
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

// ---------------- ClientSender ----------------
ClientSender::ClientSender(Protocol &proto, Queue<ServerMessage> &ob) : protocol(proto), outbox(ob) {}

void ClientSender::run()
{
    try
    {
        while (should_keep_running())
        {
            ServerMessage response;
            try
            {
                response = outbox.pop(); // bloqueante
            }
            catch (const ClosedQueue &)
            {
                // La cola fue cerrada: salimos del loop
                break;
            }
            
            // Enviar directamente el mensaje unificado
            protocol.sendMessage(response);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Sender] Exception: " << e.what() << std::endl;
    }
}

// ---------------- ClientHandler ----------------
ClientHandler::ClientHandler(Socket &&p, int id, Queue<ClientHandlerMessage> &global_inbox) : protocol(std::move(p)),
                                                                                       outbox(std::make_shared<Queue<ServerMessage>>(100)), // bounded queue tamaño 100
                                                                                       global_inbox(global_inbox),
                                                                                       sender(protocol, *outbox),
                                                                                       client_id(id),
                                                                                       receiver(protocol, id, global_inbox)
{
}

void ClientHandler::start()
{
    sender.start();
    receiver.start();
}

void ClientHandler::stop()
{
    try
    {
        protocol.shutdown();
    }
    catch (...)
    {
    }

    try
    {
        if (outbox) {
            try { outbox->close(); } catch (...) {}
        }
    }
    catch (...)
    {
    }

    sender.stop();
    receiver.stop();
}

bool ClientHandler::is_alive() { return sender.is_alive() && receiver.is_alive(); }

void ClientHandler::join()
{
    sender.join();
    receiver.join();
    // liberar la outbox para que GameLoop ya no la use
    outbox.reset();
}

std::shared_ptr<Queue<ServerMessage>> ClientHandler::get_outbox() { return outbox; }

int ClientHandler::get_id() { return client_id; }
