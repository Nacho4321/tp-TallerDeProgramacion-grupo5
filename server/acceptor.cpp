#include "acceptor.h"
#include "message_handler.h"
#include <algorithm>

Acceptor::Acceptor(Socket &acc, MessageHandler &msg_admin) 
    : acceptor(std::move(acc)), message_handler(msg_admin) {}

Acceptor::Acceptor(const char *port, MessageHandler &msg_admin) 
    : acceptor(Socket(port)), message_handler(msg_admin) {}

void Acceptor::run()
{
    while (should_keep_running())
    {
        try
        {
            reap();

            Socket peer = acceptor.accept();

            auto c = std::make_unique<ClientHandler>(std::move(peer), message_handler);
            c->start();

            clients.push_back(std::move(c));
        }
        catch (...)
        {
            if (!should_keep_running())
                break;
        }
    }
    kill_all();
}

void Acceptor::stop()
{
    Thread::stop(); // marca should_keep_running = false
    try
    {
        acceptor.shutdown(2);
        acceptor.close(); // cerramos el socket para despertar accept()
    }
    catch (...)
    {
    }
}

void Acceptor::kill_all()
{
    for (auto &client : clients)
    {
        client->stop();
    }
    clients.clear(); // destruye automáticamente los ClientHandler
}

void Acceptor::reap()
{
    clients.erase(
        std::remove_if(clients.begin(), clients.end(),
                       [](const std::unique_ptr<ClientHandler> &c)
                       {
                           if (!c->is_alive())
                           {
                               c->stop();
                               return true; // marcar para eliminar
                           }
                           return false;
                       }),
        clients.end());
}
