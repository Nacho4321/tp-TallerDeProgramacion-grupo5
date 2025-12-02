#ifndef game_state_tracker_H
#define game_state_tracker_H

#include "../common/messages.h"

// Tiene el estado del juego relevante para el cliente, todas las flags que habia en client.cpp
class GameStateTracker {
private:
    bool sawStartingCountdown;
    bool sawRaceTimes;
    bool sawTotalTimes;
    bool gameEnded;
    ServerMessage lastRaceTimesMsg;
    ServerMessage lastTotalTimesMsg;

public:
    GameStateTracker();

    void onMessage(const ServerMessage& msg);

    bool hasSawStartingCountdown() const { return sawStartingCountdown; }
    bool hasSawRaceTimes() const { return sawRaceTimes; }
    bool hasSawTotalTimes() const { return sawTotalTimes; }
    bool hasGameEnded() const { return gameEnded; }

    bool shouldExitGame(uint8_t opcode) const;

    bool shouldShowResults() const;

    const ServerMessage& getLastRaceTimesMsg() const { return lastRaceTimesMsg; }
    const ServerMessage& getLastTotalTimesMsg() const { return lastTotalTimesMsg; }

    void resetFrameState();

    bool shouldTriggerCountdown(bool got_message, uint8_t latest_opcode) const;
};

#endif // game_state_tracker_H
