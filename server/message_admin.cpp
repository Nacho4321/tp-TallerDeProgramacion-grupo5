#include "message_admin.h"

void MessageAdmin::run()
{
    init_dispatch();
    while (should_keep_running())
    {
        handle_message();
    }
}
void MessageAdmin::handle_message()
{
    ClientHandlerMessage message = global_inbox.pop();
    auto it = cli_comm_dispatch.find(message.msg.cmd);
    std::cout << message.msg.cmd << std::endl;
    if (it != cli_comm_dispatch.end())
    {
        it->second(message);
    }
    else
    {
        Event event = Event{message.client_id, message.msg.cmd};
        // TODO: encontrar forma de saber como mandar eventos
        game_queues[1]->push(event);
    }
}

void MessageAdmin::init_dispatch()
{
    cli_comm_dispatch[CREATE_GAME_STR] = [this](ClientHandlerMessage &message)
    { create_game(message); };
    cli_comm_dispatch[JOIN_GAME_STR] = [this](ClientHandlerMessage &message)
    { join_game(message); };
}

void MessageAdmin::create_game(ClientHandlerMessage &message)
{
    games_monitor.add_game(message.client_id);
}

void MessageAdmin::join_game(ClientHandlerMessage &message)
{
    games_monitor.join_player(message.client_id, message.game_id);
}
