#include "acceptor.h"
#include "lobby_handler.h"
#include <algorithm>

Acceptor::Acceptor(Socket &acc, LobbyHandler &msg_admin) 
    : acceptor(std::move(acc)), message_handler(msg_admin) {}

Acceptor::Acceptor(const char *port, LobbyHandler &msg_admin) 
    : acceptor(Socket(port)), message_handler(msg_admin) {}

void Acceptor::run()
{
    while (should_keep_running())
    {
        try
        {
            reap(); // limpiar clientes muertos

            std::cout << "[Acceptor] Esperando conexiones en el puerto..." << std::endl;
            Socket peer = acceptor.accept(); // bloqueante, espera cliente

            auto c = std::make_unique<ClientHandler>(std::move(peer), message_handler);
            c->start();

            clients.push_back(std::move(c));
        }
        catch (...)
        {
            if (!should_keep_running())
                break; // socket cerrado por stop()
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
