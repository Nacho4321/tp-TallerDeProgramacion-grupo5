#include "GameLauncher.h"
#include "../client/client.h"
#include <iostream>

int GameLauncher::launchGame(const std::string& address, const std::string& port) {
    try {
        // Lanza el cliente en modo AUTO_CREATE
        // Automáticamente crea la partida y entra al juego
        std::cout << "[GameLauncher] Launching game in AUTO_CREATE mode" << std::endl;
        Client client(address.c_str(), port.c_str(), StartMode::AUTO_CREATE);
        client.start();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Launching error: " << e.what() << std::endl;
        return 1;
    }
}

int GameLauncher::launchGameWithJoin(const std::string& address, const std::string& port, int game_id) {
    try {
        // Lanza el cliente en modo AUTO_JOIN con el game_id especificado
        // Automáticamente se une a la partida y entra al juego
        std::cout << "[GameLauncher] Launching game in AUTO_JOIN mode with game_id=" << game_id << std::endl;
        Client client(address.c_str(), port.c_str(), StartMode::AUTO_JOIN, game_id);
        client.start();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Launching error (join): " << e.what() << std::endl;
        return 1;
    }
}
