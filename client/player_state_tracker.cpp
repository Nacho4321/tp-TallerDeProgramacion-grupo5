#include "player_state_tracker.h"

PlayerStateTracker::PlayerStateTracker()
    : playerId(-1),
      originalPlayerId(-1)
{
}

void PlayerStateTracker::setPlayerId(int32_t id)
{
    playerId = id;
}

void PlayerStateTracker::setOriginalPlayerId(int32_t id)
{
    originalPlayerId = id;
}

void PlayerStateTracker::switchToSpectatorMode()
{
    playerId = SPECTATOR_MODE;
}

PlayerStateTracker::PlayerInfo PlayerStateTracker::findPlayerInPositions(const ServerMessage& msg) const
{
    PlayerInfo info;
    info.idx_main = 0;
    info.player_found = false;

    if (playerId >= 0)
    {
        for (size_t i = 0; i < msg.positions.size(); ++i)
        {
            if (msg.positions[i].player_id == playerId)
            {
                info.idx_main = i;
                info.player_found = true;
                break;
            }
        }
    }
    else
    {
        info.player_found = true;
    }

    return info;
}

PlayerStateTracker::DeathState PlayerStateTracker::checkDeathState(
    bool player_found, bool mainCarExists,
    bool isExploding, bool explosionComplete) const
{
    if (!player_found && playerId >= 0)
    {
        if (mainCarExists && !isExploding)
        {
            return DeathState::NEEDS_EXPLOSION;
        }

        if (mainCarExists && explosionComplete)
        {
            return DeathState::TRANSITION_COMPLETE;
        }

        return DeathState::EXPLODING;
    }

    return DeathState::ALIVE;
}
