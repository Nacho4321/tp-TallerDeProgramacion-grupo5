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
    int getIndex() const;

    void setIsStart(bool start);
    void setIsFinish(bool finish);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
};

#endif 
