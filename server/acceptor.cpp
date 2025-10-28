#include "acceptor.h"

Acceptor::Acceptor(Socket &&acc, Queue<IncomingMessage> &global_inbox) : acceptor(std::move(acc)), global_inbox(global_inbox) {}

Acceptor::Acceptor(const char *port, Queue<IncomingMessage> &global_inbox) : acceptor(Socket(port)), global_inbox(global_inbox) {}

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

            std::lock_guard<std::mutex> lock(mtx);
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
    std::lock_guard<std::mutex> lock(mtx);
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
    std::lock_guard<std::mutex> lock(mtx);
    for (auto it = clients.begin(); it != clients.end();)
    {
        auto &c = *it;
        if (!c->is_alive())
        {
            outbox_monitor.remove(c->get_outbox());
            c->stop();
            c->join();
            it = clients.erase(it); // se destruye automáticamente
        }
        else
        {
            ++it;
        }
    }
}

std::vector<int> Acceptor::get_client_ids()
{
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<int> ids;
    for (auto &c : clients)
        ids.push_back(c->get_id());
    return ids;
}

void Acceptor::send_to_client(int client_id, const OutgoingMessage &msg)
{
    std::lock_guard<std::mutex> lock(mtx);
    for (auto &c : clients)
    {
        if (c->get_id() == client_id)
        {
            c->get_outbox()->try_push(msg);
            return;
        }
    }
    std::cerr << "[Acceptor] Cliente " << client_id << " no encontrado\n";
}

void Acceptor::broadcast(const OutgoingMessage &msg)
{
    outbox_monitor.broadcast(msg);
}
