#include "checkpoint.h"
#include <cmath>
#include <algorithm>

Checkpoint::Checkpoint(const Position& pos, int checkpointOrder, bool hasNextCheckpoint)
    : position(pos), order(checkpointOrder), hasNext(hasNextCheckpoint) {}

void Checkpoint::setNextCheckpoint(const Position& nextPos) {
    nextCheckpointPos = nextPos;
    hasNext = true;
}

void Checkpoint::render(Renderer& renderer, const Vector2& camPos) const {
    int cx = std::round(position.new_X - camPos.x);
    int cy = std::round(position.new_Y - camPos.y);


    float sizeFactor = 1.0f - (order * 0.15f);  
    
    int radius = std::round(CHECKPOINT_RADIUS_PX * sizeFactor);
    
    Uint8 alpha = 255 - (order * 100); 

    renderer.SetDrawBlendMode(SDL_BLENDMODE_BLEND);
    renderer.SetDrawColor(255, 220, 0, alpha);

    for (int dy = -radius; dy <= radius; ++dy) {
        int yy = cy + dy;
        int span = std::floor(std::sqrt((double)radius*radius - (double)dy*dy));
        int xx = cx - span;
        int w = span * 2;
        renderer.FillRect(Rect(xx, yy, w, 1));
    }

    if (hasNext && order == 0) {  
        drawArrowToNext(renderer, cx, cy);
    }
}

void Checkpoint::drawArrowToNext(Renderer& renderer, int cx, int cy) const {
    float dx = nextCheckpointPos.new_X - position.new_X;
    float dy = nextCheckpointPos.new_Y - position.new_Y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist < 1.0f) return; 

    dx /= dist;
    dy /= dist;

    renderer.SetDrawColor(0, 100, 255, 255);
    int size = 12;
    int offsetTipX = cx + static_cast<int>(dx * size / 2);
    int offsetTipY = cy + static_cast<int>(dy * size / 2);

    drawArrowhead(renderer, offsetTipX, offsetTipY, dx, dy, 12);
}

void Checkpoint::drawArrowhead(Renderer& renderer, int tipX, int tipY, float dirX, float dirY, int size) const {
    float perpX = -dirY;
    float perpY = dirX;

    int backDist = size;
    int sideDist = size / 2;

    int baseX = tipX - static_cast<int>(dirX * backDist);
    int baseY = tipY - static_cast<int>(dirY * backDist);

    int leftX = baseX + static_cast<int>(perpX * sideDist);
    int leftY = baseY + static_cast<int>(perpY * sideDist);
    
    int rightX = baseX - static_cast<int>(perpX * sideDist);
    int rightY = baseY - static_cast<int>(perpY * sideDist);

    for (int i = -1; i <= 1; ++i) {
        renderer.DrawLine(tipX + i, tipY, leftX + i, leftY);
        renderer.DrawLine(tipX, tipY + i, leftX, leftY + i);
        
        renderer.DrawLine(tipX + i, tipY, rightX + i, rightY);
        renderer.DrawLine(tipX, tipY + i, rightX, rightY + i);    }
}

void Checkpoint::renderScreenIndicator(Renderer& renderer, const Position& checkpointPos, 
                                      const Vector2& camPos, int screenWidth, int screenHeight) {
    float cameraCenterX = camPos.x + screenWidth / 2.0f;
    float cameraCenterY = camPos.y + screenHeight / 2.0f;
    
    float dx = checkpointPos.new_X - cameraCenterX;
    float dy = checkpointPos.new_Y - cameraCenterY;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist < 1.0f) return;
    
    dx /= dist;
    dy /= dist;
    
    int indicatorX = screenWidth / 2;
    int indicatorY = 40;
    
    renderer.SetDrawBlendMode(SDL_BLENDMODE_BLEND);
    renderer.SetDrawColor(255, 220, 0, 255);
    
    float perpX = -dy;
    float perpY = dx;
    
    int headSize = 15;
    int backDist = headSize;
    int sideDist = headSize / 2;
    
    int baseX = indicatorX - static_cast<int>(dx * backDist);
    int baseY = indicatorY - static_cast<int>(dy * backDist);
    
    int leftX = baseX + static_cast<int>(perpX * sideDist);
    int leftY = baseY + static_cast<int>(perpY * sideDist);
    
    int rightX = baseX - static_cast<int>(perpX * sideDist);
    int rightY = baseY - static_cast<int>(perpY * sideDist);
    
    for (int i = -2; i <= 2; ++i) {
        renderer.DrawLine(indicatorX + i, indicatorY, leftX + i, leftY);
        renderer.DrawLine(indicatorX, indicatorY + i, leftX, leftY + i);
        
        renderer.DrawLine(indicatorX + i, indicatorY, rightX + i, rightY);
        renderer.DrawLine(indicatorX, indicatorY + i, rightX, rightY + i);
    }
}