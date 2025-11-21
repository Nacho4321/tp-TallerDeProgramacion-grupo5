#ifndef GAME_LAUNCHER_H
#define GAME_LAUNCHER_H

#include <string>

class GameLauncher {
public:
    static int launchGame(const std::string& address, const std::string& port);
    static int launchGameWithJoin(const std::string& address, const std::string& port, int game_id);
};

#endif
