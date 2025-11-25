#ifndef CHECKPOINT_ITEM_H
#define CHECKPOINT_ITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

class CheckpointItem : public QGraphicsRectItem {
private:
    int checkpointIndex; 
    bool isStart;
    bool isFinish;

public:
    CheckpointItem(float x, float y, int index, QGraphicsItem* parent = nullptr);

    void setIndex(int index);
    int getIndex() const { return checkpointIndex; }

    void setIsStart(bool start) { isStart = start; update(); }
    void setIsFinish(bool finish) { isFinish = finish; update(); }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
};

#endif 
