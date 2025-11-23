#include "GameLauncher.h"
#include "../client/client.h"
#include <iostream>

int GameLauncher::launchGame(const std::string& address, const std::string& port) {
    return launchGameInternal(address, port, StartMode::AUTO_CREATE, -1);
}


int GameLauncher::launchGameWithJoin(const std::string& address, const std::string& port, int game_id) {
    return launchGameInternal(address, port, StartMode::AUTO_JOIN, game_id);
}


int GameLauncher::launchGameInternal(const std::string& address, const std::string& port, 
                                      StartMode mode, int game_id) {
    try {
        Client client(address.c_str(), port.c_str(), mode, game_id);
        client.start();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[GameLauncher] Launching error: " << e.what() << std::endl;
        return 1;
    }
}
