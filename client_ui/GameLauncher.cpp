#include "GameLauncher.h"
#include "../client/client.h"
#include <iostream>

int GameLauncher::launchGame(const std::string& address, const std::string& port) {
    try {
        Client client(address.c_str(), port.c_str());
        client.start();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Launching error: " << e.what() << std::endl;
        return 1;
    }
}
