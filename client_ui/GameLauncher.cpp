#include "GameLauncher.h"
#include "../client/client.h"
#include <iostream>

int GameLauncher::launchGame(const std::string& address, const std::string& port) {
    try {
        Client client(address.c_str(), port.c_str(), StartMode::AUTO_CREATE);
        client.start();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Launching error (create): " << e.what() << std::endl;
        return 1;
    }
}

int GameLauncher::launchGameWithJoin(const std::string& address, const std::string& port, int game_id) {
    try {
        Client client(address.c_str(), port.c_str(), StartMode::AUTO_JOIN, game_id);
        client.start();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Launching error (join): " << e.what() << std::endl;
        return 1;
    }
}
