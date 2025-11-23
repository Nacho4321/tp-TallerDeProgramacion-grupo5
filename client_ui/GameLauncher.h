#ifndef GAME_LAUNCHER_H
#define GAME_LAUNCHER_H

#include <string>
#include "../client/client.h"

class GameLauncher {
public:
    static int launchGame(const std::string& address, const std::string& port, 
                         const std::string& game_name = "");
    static int launchGameWithJoin(const std::string& address, const std::string& port, int game_id);

private:
    static int launchGameInternal(const std::string& address, const std::string& port, 
                                   StartMode mode, int game_id, const std::string& game_name);
};

#endif
