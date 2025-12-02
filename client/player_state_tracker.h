#ifndef PLAYER_STATE_TRACKER_H
#define PLAYER_STATE_TRACKER_H

#include "../common/messages.h"
#include <cstddef>


class PlayerStateTracker {
public:
    enum class DeathState {
        ALIVE,              
        NEEDS_EXPLOSION,    
        EXPLODING,          
        TRANSITION_COMPLETE
    };

    struct PlayerInfo {
        size_t idx_main;
        bool player_found;
    };

private:
    int32_t playerId;
    int32_t originalPlayerId;
    const int SPECTATOR_MODE = -1;

public:
    PlayerStateTracker();

    void setPlayerId(int32_t id);
    void setOriginalPlayerId(int32_t id);
    void switchToSpectatorMode();

    int32_t getPlayerId() const { return playerId; }
    int32_t getOriginalPlayerId() const { return originalPlayerId; }
    bool isInSpectatorMode() const { return playerId == SPECTATOR_MODE; }

    PlayerInfo findPlayerInPositions(const ServerMessage& msg) const;

    DeathState checkDeathState(bool player_found, bool mainCarExists,
                                bool isExploding, bool explosionComplete) const;
};

#endif // PLAYER_STATE_TRACKER_H
