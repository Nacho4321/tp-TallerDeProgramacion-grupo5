#include "GameLauncher.h"
#include "../client/client.h"
#include <iostream>

int GameLauncher::launchGame(const std::string& address, const std::string& port, 
                            const std::string& game_name) {
    return launchGameInternal(address, port, StartMode::AUTO_CREATE, -1, game_name);
}


int GameLauncher::launchGameWithJoin(const std::string& address, const std::string& port, int game_id) {
    return launchGameInternal(address, port, StartMode::AUTO_JOIN, game_id, "");
}


int GameLauncher::launchGameInternal(const std::string& address, const std::string& port, 
                                      StartMode mode, int game_id, const std::string& game_name) {
    try {
        Client client(address.c_str(), port.c_str(), mode, game_id, game_name);
        client.start();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[GameLauncher] Launching error: " << e.what() << std::endl;
        return 1;
    }
}
