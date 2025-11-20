#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include <SDL2pp/SDL2pp.hh>
#include "../common/Event.h"
#include "../common/constants.h"
#include "vector2.h"

using namespace SDL2pp;

class Checkpoint {
private:
    Position position;
    int order;  // 0 = first checkpoint
    Position nextCheckpointPos;  
    bool hasNext;

public:
    Checkpoint(const Position& pos, int checkpointOrder, bool hasNextCheckpoint = false);
    
    void setNextCheckpoint(const Position& nextPos);
    void render(Renderer& renderer, const Vector2& camPos) const;
    const Position& getPosition() const { return position; }

    static void renderScreenIndicator(Renderer& renderer, const Position& checkpointPos, const Vector2& camPos, int screenWidth, int screenHeight);

private:
    void drawArrowToNext(Renderer& renderer, int cx, int cy) const;
    void drawArrowhead(Renderer& renderer, int tipX, int tipY, float dirX, float dirY, int size) const;
};

#endif // CHECKPOINT_H