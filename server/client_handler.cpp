#include "client_handler.h"

// ---------------- ClientReceiver ----------------
ClientReceiver::ClientReceiver(Protocol& proto, int id, Queue<IncomingMessage>& global_inbox):
        protocol(proto), client_id(id), global_inbox(global_inbox) {}

void ClientReceiver::run() {
    try {
        while (should_keep_running()) {
            DecodedMessage dec_msg = protocol.receiveMessage();
            if (dec_msg.cmd.empty())
                break;  // EOF o desconexión

            std::cout << "[Server] Client " << client_id << " sent: " 
                      << dec_msg.cmd << std::endl;  // DEBUG

            IncomingMessage msg =
                    make_message_from_decoded(dec_msg);  // Basicamente le agrega el client id
            global_inbox.push(msg);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Receiver] Exception: " << e.what() << std::endl;
    }
}

IncomingMessage ClientReceiver::make_message_from_decoded(const DecodedMessage& cmd) {
    IncomingMessage im;
    im.cmd = cmd.cmd;
    im.client_id = this->client_id;
    return im;
}

// ---------------- ClientSender ----------------
ClientSender::ClientSender(Protocol& proto, Queue<OutgoingMessage>& ob):
        protocol(proto), outbox(ob) {}

void ClientSender::run() {
    try {
        while (should_keep_running()) {
            OutgoingMessage msg;
            try {
                msg = outbox.pop();  // bloqueante
            } catch (const ClosedQueue&) {
                // La cola fue cerrada: salimos del loop
                break;
            }
            protocol.sendMessage(msg.cmd);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Sender] Exception: " << e.what() << std::endl;
    }
}

// ---------------- ClientHandler ----------------
ClientHandler::ClientHandler(Socket&& p, int id, Queue<IncomingMessage>& global_inbox):
        protocol(std::move(p)),
        outbox(std::make_shared<Queue<OutgoingMessage>>(100)),  // bounded queue tamaño 100
        global_inbox(global_inbox),
        sender(protocol, *outbox),
        client_id(id),
        receiver(protocol, id, global_inbox) {}

void ClientHandler::start() {
    sender.start();
    receiver.start();
}

void ClientHandler::stop() {
    try {
        protocol.shutdown();
    } catch (...) {}

    try {
        outbox->close();
    } catch (...) {}

    sender.stop();
    receiver.stop();
}

bool ClientHandler::is_alive() { return sender.is_alive() && receiver.is_alive(); }

void ClientHandler::join() {
    sender.join();
    receiver.join();
}

std::shared_ptr<Queue<OutgoingMessage>> ClientHandler::get_outbox() { return outbox; }

int ClientHandler::get_id() { return client_id; }
