#include "message_admin.h"

void MessageAdmin::run()
{
    init_dispatch();
    while (should_keep_running())
    {
        try {
            handle_message();
        } catch (const ClosedQueue&) {
            // Alguna cola se cerró (por ejemplo, de un juego terminado). Continuamos atendiendo otros mensajes.
            continue;
        } catch (const std::exception& e) {
            std::cerr << "[MessageAdmin] Unexpected exception: " << e.what() << std::endl;
        }
    }
}
void MessageAdmin::handle_message()
{
    ClientHandlerMessage message;
    // pop puede lanzar ClosedQueue si la cola fue cerrada al apagar el servidor
    message = global_inbox.pop();
    auto it = cli_comm_dispatch.find(message.msg.cmd);
    std::cout << message.msg.cmd << std::endl;
    if (it != cli_comm_dispatch.end())
    {
        it->second(message);
    }
    else
    {
        // Eventos de juego (movimientos, etc.)
        Event event = Event{message.client_id, message.msg.cmd};
        int target_gid = message.msg.game_id;
        std::shared_ptr<Queue<Event>> target_q;
        {
            std::lock_guard<std::mutex> lk(game_queues_mutex);
            if (target_gid > 0) {
                auto itq = game_queues.find(target_gid);
                if (itq != game_queues.end()) target_q = itq->second;
            }
            // Fallback: si no vino game_id o no existe, y hay exactamente 1 juego, usar ese
            if (!target_q && game_queues.size() == 1) {
                target_q = game_queues.begin()->second;
            }
        }
        if (target_q) {
            try {
                target_q->push(event);
            } catch (const ClosedQueue&) {
                // La cola del juego pudo haberse cerrado: ignoramos este evento
            }
        } else {
            std::cerr << "[MessageAdmin] WARNING: No se encontró cola para game_id="
                      << target_gid << ", evento='" << message.msg.cmd
                      << "' desde client=" << message.client_id << std::endl;
        }
    }
}

void MessageAdmin::init_dispatch()
{
    cli_comm_dispatch[CREATE_GAME_STR] = [this](ClientHandlerMessage &message)
    { create_game(message); };
    cli_comm_dispatch[JOIN_GAME_STR] = [this](ClientHandlerMessage &message)
    { join_game(message); };
    cli_comm_dispatch[GET_GAMES_STR] = [this](ClientHandlerMessage &message)
    { get_games(message); };
}

void MessageAdmin::create_game(ClientHandlerMessage &message)
{
    std::cout << "[MessageAdmin] Cliente " << message.client_id << " solicita crear partida" << std::endl;
    int game_id = games_monitor.add_game(message.client_id);
    
    // Enviar respuesta al cliente con los IDs asignados
    ServerMessage response;
    response.opcode = GAME_JOINED;
    response.game_id = static_cast<uint32_t>(game_id);
    response.player_id = static_cast<uint32_t>(message.client_id);
    response.success = true;
    
    std::cout << "[MessageAdmin] Enviando respuesta: game_id=" << game_id 
              << " player_id=" << message.client_id << std::endl;
    
    auto client_queue = outboxes.get_cliente_queue(message.client_id);
    if (client_queue) {
        try {
            std::cout << "[MessageAdmin] Pushing GameJoined (ServerMessage) al outbox del cliente " << message.client_id << std::endl;
            client_queue->push(response);
            std::cout << "[MessageAdmin] Respuesta enviada a la cola del cliente" << std::endl;
        } catch (const ClosedQueue&) {
            std::cerr << "[MessageAdmin] Cola cerrada para cliente " << message.client_id << ", descartando respuesta" << std::endl;
            outboxes.remove(message.client_id);
        }
    } else {
        std::cerr << "[MessageAdmin] ERROR: No se encontró cola para cliente " << message.client_id << std::endl;
    }
}

void MessageAdmin::join_game(ClientHandlerMessage &message)
{
    // Validar que el juego existe antes de intentar unirse
    ServerMessage response;
    try {
        games_monitor.join_player(message.client_id, message.msg.game_id);
        response.opcode = GAME_JOINED;
        response.game_id = static_cast<uint32_t>(message.msg.game_id);
        response.player_id = static_cast<uint32_t>(message.client_id);
        response.success = true;
    } catch (...) {
        // Fallo al unirse (juego no existe u otro error)
        response.opcode = GAME_JOINED;
        response.game_id = 0;
        response.player_id = 0;
        response.success = false;
    }
    
    auto client_queue = outboxes.get_cliente_queue(message.client_id);
    if (client_queue) {
        try {
            std::cout << "[MessageAdmin] JoinGame resp para cliente " << message.client_id
                      << ": success=" << std::boolalpha << response.success
                      << " game_id=" << response.game_id
                      << " player_id=" << response.player_id << std::endl;
            std::cout << "[MessageAdmin] Pushing JoinGame (ServerMessage) al outbox del cliente " << message.client_id << std::endl;
            client_queue->push(response);
        } catch (const ClosedQueue&) {
            outboxes.remove(message.client_id);
        }
    }
}

void MessageAdmin::get_games(ClientHandlerMessage &message) {
    (void)message; // no necesitamos datos extra del cliente por ahora
    ServerMessage resp; resp.opcode = GAMES_LIST; resp.games = games_monitor.list_games();
    // Enviar listado a este cliente
    auto client_queue = outboxes.get_cliente_queue(message.client_id);
    if (client_queue) {
        try { client_queue->push(resp); } catch (const ClosedQueue&) { outboxes.remove(message.client_id); }
    }
}
