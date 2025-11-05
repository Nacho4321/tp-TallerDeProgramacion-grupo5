#include "acceptor.h"

Acceptor::Acceptor(Socket &acc, Queue<ClientHandlerMessage> &global_inbox, OutboxMonitor &outboxes) : acceptor(std::move(acc)), global_inbox(global_inbox), outbox_monitor(outboxes) {}

Acceptor::Acceptor(const char *port, Queue<ClientHandlerMessage> &global_inbox, OutboxMonitor &outboxes) : acceptor(Socket(port)), global_inbox(global_inbox), outbox_monitor(outboxes) {}

void Acceptor::run()
{
    while (should_keep_running())
    {
        try
        {
            Socket peer = acceptor.accept(); // bloqueante, espera cliente

            int id = next_id++;
            auto c = std::make_unique<ClientHandler>(std::move(peer), id, global_inbox);

            reap(); // limpiar clientes muertos

            outbox_monitor.add(c->get_outbox());
            c->start();

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
    for (auto &client : clients)
    {
        try
        {
            outbox_monitor.remove(client->get_outbox());
            client->stop();
        }
        catch (...)
        {
        }
    }

    for (auto &client : clients)
    {
        try
        {
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
            outbox_monitor.remove(c->get_outbox());
            c->stop();
            c->join();
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
