#include "game_state_tracker.h"
#include "../common/constants.h"

GameStateTracker::GameStateTracker()
    : sawStartingCountdown(false),
      sawRaceTimes(false),
      sawTotalTimes(false),
      gameEnded(false)
{
}

void GameStateTracker::onMessage(const ServerMessage& msg)
{
    if (msg.opcode == STARTING_COUNTDOWN)
    {
        sawStartingCountdown = true;
    }
    else if (msg.opcode == RACE_TIMES)
    {
        sawRaceTimes = true;
        lastRaceTimesMsg = msg;
    }
    else if (msg.opcode == TOTAL_TIMES)
    {
        sawTotalTimes = true;
        lastTotalTimesMsg = msg;
        gameEnded = true;
    }
}

bool GameStateTracker::shouldExitGame(uint8_t opcode) const
{
    if (opcode == GAME_STARTED) {
        if (gameEnded && (opcode == GAME_STARTED)) {
            return true;
        }
    }
    return false;
}

bool GameStateTracker::shouldShowResults() const
{
    return sawRaceTimes || sawTotalTimes;
}

void GameStateTracker::resetFrameState()
{
    sawStartingCountdown = false;
    sawRaceTimes = false;
    sawTotalTimes = false;
}

bool GameStateTracker::shouldTriggerCountdown(bool got_message, uint8_t latest_opcode) const
{
    return got_message && (latest_opcode == STARTING_COUNTDOWN || sawStartingCountdown);
}
