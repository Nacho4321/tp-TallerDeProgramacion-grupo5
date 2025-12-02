#include "checkpoint_item.h"
#include <QBrush>
#include <QPen>
#include <QFont>

namespace {
    constexpr float CHECKPOINT_WIDTH = 80.0f;
    constexpr float CHECKPOINT_HEIGHT = 20.0f;
    
    const QColor COLOR_START(0, 255, 0, 180);      
    const QColor COLOR_FINISH(255, 0, 0, 180);     
    const QColor COLOR_INTERMEDIATE(255, 255, 0, 180); 
}

CheckpointItem::CheckpointItem(float x, float y, int index, QGraphicsItem* parent)
    : QGraphicsRectItem(x - CHECKPOINT_WIDTH / 2, y - CHECKPOINT_HEIGHT / 2,
                        CHECKPOINT_WIDTH, CHECKPOINT_HEIGHT, parent),
      checkpointIndex(index),
      isStart(false),
      isFinish(false) {
    
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    
    setPen(QPen(Qt::yellow, 2));
    setBrush(QBrush(COLOR_INTERMEDIATE));
}

void CheckpointItem::setIndex(int index) {
    checkpointIndex = index;
    update();
}

int CheckpointItem::getIndex() const {
    return checkpointIndex;
}

void CheckpointItem::setIsStart(bool start) {
    isStart = start;
    update();
}

void CheckpointItem::setIsFinish(bool finish) {
    isFinish = finish;
    update();
}

void CheckpointItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, 
                           QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    QColor color;
    QString text;
    
    if (isStart) {
        color = COLOR_START;
        text = "S";
    } else if (isFinish) {
        color = COLOR_FINISH;
        text = "F";
    } else {
        color = COLOR_INTERMEDIATE;
        text = QString::number(checkpointIndex + 1);
    }
    

    painter->setPen(QPen(color.darker(), 2));
    painter->setBrush(QBrush(color));
    painter->drawRect(rect());
    

    painter->setPen(Qt::black);
    painter->setFont(QFont("Arial", 12, QFont::Bold));
    painter->drawText(rect(), Qt::AlignCenter, text);
    

    if (isSelected()) {
        painter->setPen(QPen(Qt::white, 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect());
    }
}

void CheckpointItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsRectItem::mousePressEvent(event);
}

void CheckpointItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsRectItem::mouseReleaseEvent(event);
}
