#include "lobby_handler.h"

LobbyHandler::LobbyHandler(GameMonitor &games_mon)
    : games_monitor(games_mon),
      lobby_command_handlers()
{
    init_dispatch();
}

void LobbyHandler::handle_message(ClientHandlerMessage &message)
{
    auto it = lobby_command_handlers.find(message.msg.cmd);
    if (it != lobby_command_handlers.end())
    {
        it->second(message);
    }
    else
    {
        Event event = Event{message.client_id, message.msg.cmd};
        int target_gid = message.msg.game_id;
        std::shared_ptr<Queue<Event>> target_q = games_monitor.get_game_queue(target_gid);

        if (target_q)
        {
            try
            {
                target_q->push(event);
            }
            catch (const ClosedQueue &)
            {
                // La cola del juego pudo haberse cerrado: ignoramos este evento
            }
        }
        else
        {
            std::cerr << "[LobbyHandler] WARNING: No se encontró cola para game_id="
                      << target_gid << ", evento='" << message.msg.cmd
                      << "' desde client=" << message.client_id << std::endl;
        }
    }
}

void LobbyHandler::init_dispatch()
{
    lobby_command_handlers[CREATE_GAME_STR] = [this](ClientHandlerMessage &message)
    { create_game(message); };
    lobby_command_handlers[JOIN_GAME_STR] = [this](ClientHandlerMessage &message)
    { join_game(message); };
    lobby_command_handlers[GET_GAMES_STR] = [this](ClientHandlerMessage &message)
    { get_games(message); };
    lobby_command_handlers[START_GAME_STR] = [this](ClientHandlerMessage &message)
    { start_game(message); };
    lobby_command_handlers[LEAVE_GAME_STR] = [this](ClientHandlerMessage &message)
    { leave_game(message); };
}

void LobbyHandler::create_game(ClientHandlerMessage &message)
{

    // Obtener outbox del mensaje
    auto client_queue = message.outbox;
    if (!client_queue)
    {
        std::cerr << "[LobbyHandler] ERROR: No se encontró outbox en el mensaje" << std::endl;
        return;
    }

    int game_id = games_monitor.add_game(message.client_id, client_queue, message.msg.game_name, message.msg.map_id);

    ServerMessage response;
    response.opcode = GAME_JOINED;
    response.game_id = static_cast<uint32_t>(game_id);
    response.player_id = static_cast<uint32_t>(message.client_id);
    response.success = true;
    response.map_id = message.msg.map_id;

    try
    {
        client_queue->push(response);
    }
    catch (const ClosedQueue &)
    {
        std::cerr << "[LobbyHandler] Cola cerrada para cliente " << message.client_id << ", descartando respuesta" << std::endl;
    }
}

void LobbyHandler::join_game(ClientHandlerMessage &message)
{
    ServerMessage response;

    auto client_queue = message.outbox;
    if (!client_queue)
    {
        response.opcode = GAME_JOINED;
        response.game_id = 0;
        response.player_id = 0;
        response.success = false;
        return;
    }

    try
    {
        games_monitor.join_player(message.client_id, message.msg.game_id, client_queue);
        response.opcode = GAME_JOINED;
        response.game_id = static_cast<uint32_t>(message.msg.game_id);
        response.player_id = static_cast<uint32_t>(message.client_id);
        response.success = true;
        response.map_id = games_monitor.get_game_map_id(message.msg.game_id);
    }
    catch (...)
    {
        response.opcode = GAME_JOINED;
        response.game_id = 0;
        response.player_id = 0;
        response.success = false;
        response.map_id = 0;
    }

    try
    {
        client_queue->push(response);
    }
    catch (const ClosedQueue &)
    {
        std::cerr << "[LobbyHandler] Cola cerrada para cliente " << message.client_id << std::endl;
    }
}

void LobbyHandler::get_games(ClientHandlerMessage &message)
{
    (void)message;
    ServerMessage resp;
    resp.opcode = GAMES_LIST;
    resp.games = games_monitor.list_games();
    auto client_queue = message.outbox;
    if (client_queue)
    {
        try
        {
            client_queue->push(resp);
        }
        catch (const ClosedQueue &)
        { /* Cola cerrada */
        }
    }
}

void LobbyHandler::start_game(ClientHandlerMessage &message)
{
    GameLoop *game = games_monitor.get_game(message.msg.game_id);
    if (!game)
    {
        return;
    }

    try
    {
        game->start_game();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[LobbyHandler] Error al iniciar partida: " << e.what() << std::endl;
    }
}

void LobbyHandler::leave_game(ClientHandlerMessage &message)
{
    // Primero remover al jugador de la partida (si está en alguna)
    games_monitor.remove_player(message.client_id);

    // Luego cerrar su outbox
    if (message.outbox)
    {
        try
        {
            message.outbox->close();
        }
        catch (const std::exception &e)
        {
            std::cerr << "[LobbyHandler] WARNING: outbox->close() lanzó excepción: " << e.what() << std::endl;
        }
    }
}
