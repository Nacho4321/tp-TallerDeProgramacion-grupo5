#include "client_handler.h"

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

    sender.join();
    receiver.join();

    outbox.reset();
}

bool ClientHandler::is_alive() { return sender.is_alive() && receiver.is_alive(); }

std::shared_ptr<Queue<ServerMessage>> ClientHandler::get_outbox() { return outbox; }

int ClientHandler::get_id() { return client_id; }
