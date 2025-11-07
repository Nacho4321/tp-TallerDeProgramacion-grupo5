#include "acceptor.h"

Acceptor::Acceptor(Socket &acc, Queue<ClientHandlerMessage> &global_inbox, OutboxMonitor &outboxes) : acceptor(std::move(acc)), global_inbox(global_inbox), outbox_monitor(outboxes) {}

Acceptor::Acceptor(const char *port, Queue<ClientHandlerMessage> &global_inbox, OutboxMonitor &outboxes) : acceptor(Socket(port)), global_inbox(global_inbox), outbox_monitor(outboxes) {}

void Acceptor::run()
{
    while (should_keep_running())
    {
        try
        {
            std::cout << "[Acceptor] Esperando conexiones en el puerto..." << std::endl;
            Socket peer = acceptor.accept(); // bloqueante, espera cliente

            int id = next_id++;
            std::cout << "[Acceptor] Cliente conectado con ID: " << id << std::endl;
            auto c = std::make_unique<ClientHandler>(std::move(peer), id, global_inbox);

            reap(); // limpiar clientes muertos

            outbox_monitor.add(c->get_id(), c->get_outbox());
            c->start();
            std::cout << "[Acceptor] ClientHandler iniciado para cliente " << id << std::endl;

            clients.push_back(std::move(c));
        }
        catch (...)
        {
            if (!should_keep_running())
                break; // error o socket cerrado
        }
    }
    clear();
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

void Acceptor::clear()
{
    outbox_monitor.remove_all();
    for (auto &client : clients)
    {
        try
        {
            client->stop();
            client->join();
        }
        catch (...)
        {
        }
    }
    clients.clear(); // se destruyen automáticamente los ClientHandler
}

void Acceptor::reap()
{
    for (auto it = clients.begin(); it != clients.end();)
    {
        auto &c = *it;
        if (!c->is_alive())
        {
            outbox_monitor.remove(c->get_id());
            c->stop();
            c->join();
            // Eliminar del vector y continuar sin incrementar el iterador
            it = clients.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Acceptor::broadcast(const ServerMessage &msg)
{
    outbox_monitor.broadcast(msg);
}
