#include "message_handler.h"

MessageHandler::MessageHandler(std::unordered_map<int, std::shared_ptr<Queue<Event>>> &game_qs,
                           std::mutex &game_qs_mutex,
                           GameMonitor &games_mon,
                           OutboxMonitor &outbox)
    : game_queues(game_qs), 
      game_queues_mutex(game_qs_mutex), 
      games_monitor(games_mon), 
      cli_comm_dispatch(), 
      outboxes(outbox) 
{
    init_dispatch();
}

void MessageHandler::handle_message(ClientHandlerMessage &message)
{
    auto it = cli_comm_dispatch.find(message.msg.cmd);
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
            std::cerr << "[MessageHandler] WARNING: No se encontró cola para game_id="
                      << target_gid << ", evento='" << message.msg.cmd
                      << "' desde client=" << message.client_id << std::endl;
        }
    }
}

void MessageHandler::init_dispatch()
{
    cli_comm_dispatch[CREATE_GAME_STR] = [this](ClientHandlerMessage &message)
    { create_game(message); };
    cli_comm_dispatch[JOIN_GAME_STR] = [this](ClientHandlerMessage &message)
    { join_game(message); };
    cli_comm_dispatch[GET_GAMES_STR] = [this](ClientHandlerMessage &message)
    { get_games(message); };
    cli_comm_dispatch[START_GAME_STR] = [this](ClientHandlerMessage &message)
    { start_game(message); };
    cli_comm_dispatch[LEAVE_GAME_STR] = [this](ClientHandlerMessage &message)
    { leave_game(message); };
}

void MessageHandler::create_game(ClientHandlerMessage &message)
{
    std::cout << "[MessageHandler] Cliente " << message.client_id << " solicita crear partida" << std::endl;
    
    // Obtener outbox del cliente
    auto client_queue = outboxes.get(message.client_id);
    if (!client_queue) {
        std::cerr << "[MessageHandler] ERROR: No se encontró cola para cliente " << message.client_id << std::endl;
        return;
    }
    
    int game_id = games_monitor.add_game(message.client_id, client_queue, message.msg.game_name);
    
    // Enviar respuesta al cliente con los IDs asignados
    ServerMessage response;
    response.opcode = GAME_JOINED;
    response.game_id = static_cast<uint32_t>(game_id);
    response.player_id = static_cast<uint32_t>(message.client_id);
    response.success = true;
    
    std::cout << "[MessageHandler] Enviando respuesta: game_id=" << game_id 
              << " player_id=" << message.client_id << std::endl;
    
    try {
        std::cout << "[MessageHandler] Pushing GameJoined (ServerMessage) al outbox del cliente " << message.client_id << std::endl;
        client_queue->push(response);
        std::cout << "[MessageHandler] Respuesta enviada a la cola del cliente" << std::endl;
    } catch (const ClosedQueue&) {
        std::cerr << "[MessageHandler] Cola cerrada para cliente " << message.client_id << ", descartando respuesta" << std::endl;
        outboxes.remove(message.client_id);
    }
}

void MessageHandler::join_game(ClientHandlerMessage &message)
{
    // Validar que el juego existe antes de intentar unirse
    ServerMessage response;
    
    // Obtener outbox del cliente
    auto client_queue = outboxes.get(message.client_id);
    if (!client_queue) {
        std::cerr << "[MessageHandler] ERROR: No se encontró cola para cliente " << message.client_id << std::endl;
        response.opcode = GAME_JOINED;
        response.game_id = 0;
        response.player_id = 0;
        response.success = false;
        return;
    }
    
    try {
        games_monitor.join_player(message.client_id, message.msg.game_id, client_queue);
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
    
    try {
        std::cout << "[MessageHandler] JoinGame resp para cliente " << message.client_id
                  << ": success=" << std::boolalpha << response.success
                  << " game_id=" << response.game_id
                  << " player_id=" << response.player_id << std::endl;
        std::cout << "[MessageHandler] Pushing JoinGame (ServerMessage) al outbox del cliente " << message.client_id << std::endl;
        client_queue->push(response);
    } catch (const ClosedQueue&) {
        outboxes.remove(message.client_id);
    }
}

void MessageHandler::get_games(ClientHandlerMessage &message) {
    (void)message; // no necesitamos datos extra del cliente por ahora
    ServerMessage resp; resp.opcode = GAMES_LIST; resp.games = games_monitor.list_games();
    // Enviar listado a este cliente
    auto client_queue = outboxes.get(message.client_id);
    if (client_queue) {
        try { client_queue->push(resp); } catch (const ClosedQueue&) { outboxes.remove(message.client_id); }
    }
}

void MessageHandler::start_game(ClientHandlerMessage &message) {
    std::cout << "[MessageHandler] Cliente " << message.client_id << " solicita iniciar partida (game_id=" << message.msg.game_id << ")" << std::endl;
    
    GameLoop* game = games_monitor.get_game(message.msg.game_id);
    if (!game) {
        std::cerr << "[MessageHandler] Error: Game " << message.msg.game_id << " not found" << std::endl;
        return;
    }
    
    try {
        game->start_game();
        std::cout << "[MessageHandler] Partida " << message.msg.game_id << " iniciada exitosamente" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[MessageHandler] Error al iniciar partida: " << e.what() << std::endl;
    }
}

void MessageHandler::leave_game(ClientHandlerMessage &message)
{
    std::cout << "[MessageHandler] Cliente " << message.client_id << " solicita dejar partida" << std::endl;
    
    // Buscar en qué juego está el jugador y removerlo
    // Como no tenemos un mapa player->game, iteramos por los juegos
    std::lock_guard<std::mutex> lk(game_queues_mutex);
    for (auto &[game_id, queue] : game_queues) {
        GameLoop* game = games_monitor.get_game(game_id);
        if (game) {
            try {
                game->remove_player(message.client_id);
                std::cout << "[MessageHandler] Cliente " << message.client_id 
                         << " removido del juego " << game_id << std::endl;
                break; // Asumimos que el jugador está en un solo juego
            } catch (...) {
                // No está en este juego, continuar
            }
        }
    }
    
    // Siempre intentar limpiar el outbox del cliente
    try {
        outboxes.remove(message.client_id);
    } catch (const std::exception &e) {
        std::cerr << "[MessageHandler] outboxes.remove threw: " << e.what() << std::endl;
    }
}
