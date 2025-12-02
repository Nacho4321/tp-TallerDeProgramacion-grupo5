#ifndef EVENTLOOP_EVENTLOOP_H
#define EVENTLOOP_EVENTLOOP_H
#include <string>
#include "../common/queue.h"
#include "game_event_handler.h"
#include "game_state.h"

// EventLoop ya NO es un Thread separado
// Se ejecuta dentro del GameLoop procesando eventos de manera sincrónica
class EventLoop
{
private:
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    std::shared_ptr<Queue<Event>> &event_queue;
    GameEventHandler dispatcher;

public:
    explicit EventLoop(std::mutex &map_mutex, std::unordered_map<int, PlayerData> &map, std::shared_ptr<Queue<Event>> &global_inb);
    
    // Procesa todos los eventos disponibles sin bloquear, respetando el estado del juego
    void process_available_events(GameState state);
    
    ~EventLoop() = default;
};
#endif