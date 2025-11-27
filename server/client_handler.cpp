#include "client_handler.h"
#include "message_handler.h"

// Inicialización del contador estático
int ClientHandler::next_id = 0;

// ---------------- ClientHandler ----------------
ClientHandler::ClientHandler(Socket &&p, MessageHandler &msg_admin) 
    : protocol(std::move(p)),
      outbox(std::make_shared<Queue<ServerMessage>>(100)), // bounded queue tamaño 100
      message_handler(msg_admin),
      sender(protocol, *outbox),
      client_id(next_id++),  // Auto-asigna ID
      receiver(protocol, client_id, msg_admin, outbox)  // Pasar outbox a receiver
{
    std::cout << "[ClientHandler] Cliente creado con ID: " << client_id << std::endl;
}

ClientHandler::~ClientHandler() = default;

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
