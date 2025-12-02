#include "spawn_formation_item.h"
#include <QStyleOptionGraphicsItem>
#include <cmath>

SpawnFormationItem::SpawnFormationItem(QGraphicsItem* parent)
    : QGraphicsItem(parent), currentOrientation(Orientation::Up) {
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

QRectF SpawnFormationItem::boundingRect() const {
    float width = COL_SPACING + CAR_WIDTH + 40;
    float height = ROW_SPACING * 3 + CAR_HEIGHT + 40;
    return QRectF(-width / 2, -height / 2, width, height);
}

QPointF SpawnFormationItem::getCarPosition(int carIndex) const {
    int row = carIndex / 2;   
    int col = carIndex % 2;   
    
    float x = (col == 0) ? -COL_SPACING / 2 : COL_SPACING / 2;
    float y = (row - 1.5f) * ROW_SPACING;
    
    float angleDegrees = static_cast<float>(currentOrientation);
    float angleRadians = angleDegrees * M_PI / 180.0f;
    
    float rotatedX = x * cos(angleRadians) - y * sin(angleRadians);
    float rotatedY = x * sin(angleRadians) + y * cos(angleRadians);
    
    return QPointF(rotatedX, rotatedY);
}

void SpawnFormationItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, 
                                QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    float angleDegrees = static_cast<float>(currentOrientation);
    
    for (int i = 0; i < SPAWN_COUNT; i++) {
        QPointF carPos = getCarPosition(i);
        
        painter->save();
        painter->translate(carPos);
        painter->rotate(angleDegrees);
        
        QRectF carRect(-CAR_WIDTH / 2, -CAR_HEIGHT / 2, CAR_WIDTH, CAR_HEIGHT);
        painter->setPen(QPen(QColor(0, 200, 200), 2));
        painter->setBrush(QBrush(QColor(0, 200, 200, 150)));
        painter->drawRect(carRect);
        
        painter->setPen(Qt::black);
        painter->setFont(QFont("Arial", 8, QFont::Bold));
        painter->drawText(carRect, Qt::AlignCenter, QString::number(i));
        
        painter->restore();
    }
    
    if (isSelected()) {
        painter->setPen(QPen(Qt::white, 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect().adjusted(5, 5, -5, -5));
    }
}

void SpawnFormationItem::setOrientation(Orientation orientation) {
    currentOrientation = orientation;
    update();
}

void SpawnFormationItem::rotateLeft() {
    int angle = static_cast<int>(currentOrientation);
    angle = (angle + 270) % 360;
    currentOrientation = static_cast<Orientation>(angle);
    update();
}

void SpawnFormationItem::rotateRight() {
    int angle = static_cast<int>(currentOrientation);
    angle = (angle + 90) % 360;
    currentOrientation = static_cast<Orientation>(angle);
    update();
}

std::vector<SpawnPointData> SpawnFormationItem::getSpawnPoints() const {
    std::vector<SpawnPointData> points;
    QPointF center = pos();
    float angle = static_cast<float>(currentOrientation);
    
    for (int i = 0; i < SPAWN_COUNT; i++) {
        QPointF carPos = getCarPosition(i);
        
        SpawnPointData data;
        data.id = i;
        data.x = center.x() + carPos.x();
        data.y = center.y() + carPos.y();
        data.angle = angle;
        data.description = "Spawn " + std::to_string(i);
        
        points.push_back(data);
    }
    
    return points;
}

void SpawnFormationItem::loadFromSpawnPoints(const std::vector<SpawnPointData>& spawnPoints) {
    if (spawnPoints.empty()) {
        return;
    }
    
    float totalX = 0;
    float totalY = 0;
    for (const auto& sp : spawnPoints) {
        totalX += sp.x;
        totalY += sp.y;
    }
    float centerX = totalX / spawnPoints.size();
    float centerY = totalY / spawnPoints.size();
    setPos(centerX, centerY);
    
    float angle = spawnPoints[0].angle;
    
    if (angle >= 315 || angle < 45) {
        currentOrientation = Orientation::Up;
    } else if (angle >= 45 && angle < 135) {
        currentOrientation = Orientation::Right;
    } else if (angle >= 135 && angle < 225) {
        currentOrientation = Orientation::Down;
    } else {
        currentOrientation = Orientation::Left;
    }
    
    update();
}

Orientation SpawnFormationItem::getOrientation() const {
    return currentOrientation;
}
