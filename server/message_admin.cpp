#include "message_admin.h"

void MessageAdmin::run()
{
    while (should_keep_running())
    {
        handle_message();
    }
}
void MessageAdmin::handle_message()
{
    ClientHandlerMessage message = global_inbox.pop();
    auto it = cli_comm_dispatch.find(message.msg.cmd);
    if (it != cli_comm_dispatch.end())
    {
        it->second(message.client_id);
    }
    else
    {
        std::cout << "Evento desconocido: " << message.msg.cmd << std::endl;
    }
}

void MessageAdmin::init_dispatch()
{
    cli_comm_dispatch[CREATE_GAME_STR] = [this](int &id)
    { create_game(id); };
    cli_comm_dispatch[JOIN_GAME_STR] = [this](int &id)
    { join_game(id); };
}

void MessageAdmin::create_game(int &client_id)
{
    games_monitor.add_game(client_id);
}

void MessageAdmin::join_game(int &client_id)
{
    games_monitor.add_game(client_id);
}
