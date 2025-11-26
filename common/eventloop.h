#ifndef EVENTLOOP_EVENTLOOP_H
#define EVENTLOOP_EVENTLOOP_H
#include <string>
#include "queue.h"
#include "eventDispatcher.h"

// EventLoop ya NO es un Thread separado
// Se ejecuta dentro del GameLoop procesando eventos de manera sincrónica
class EventLoop
{
private:
    std::mutex &players_map_mutex;
    std::unordered_map<int, PlayerData> &players;
    std::shared_ptr<Queue<Event>> &event_queue;
    EventDispatcher dispatcher;

public:
    explicit EventLoop(std::mutex &map_mutex, std::unordered_map<int, PlayerData> &map, std::shared_ptr<Queue<Event>> &global_inb) 
        : players_map_mutex(map_mutex), players(map), event_queue(global_inb), dispatcher(players_map_mutex, players) {}
    
    // Procesa todos los eventos disponibles sin bloquear
    void process_available_events();
    
    ~EventLoop() = default;
};
#endif