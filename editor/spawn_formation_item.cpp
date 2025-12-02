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
    // Bounding rect fijo que cubre todas las rotaciones posibles
    float maxSize = ROW_SPACING * 3 + CAR_HEIGHT + 50;
    return QRectF(-maxSize / 2, -maxSize / 2, maxSize, maxSize);
}

QPointF SpawnFormationItem::getCarPosition(int carIndex) const {
    // Layout base (orientación Up):
    //   [0]  [1]
    //   [2]  [3]
    //   [4]  [5]
    //   [6]  [7]
    int row = carIndex / 2;
    int col = carIndex % 2;
    
    float x = (col == 0) ? -COL_SPACING / 2 : COL_SPACING / 2;
    float y = (row - 1.5f) * ROW_SPACING;
    
    // Aplico rotación según orientación
    switch (currentOrientation) {
        case Orientation::Up:
            return QPointF(x, y);
        case Orientation::Right:
            return QPointF(-y, x);
        case Orientation::Down:
            return QPointF(-x, -y);
        case Orientation::Left:
            return QPointF(y, -x);
        default:
            return QPointF(x, y);
    }
}

void SpawnFormationItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, 
                                QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Ángulo visual para rotar los rectángulos de los autos
    float visualAngle = 0;
    switch (currentOrientation) {
        case Orientation::Up:    visualAngle = 0;   break;
        case Orientation::Right: visualAngle = 90;  break;
        case Orientation::Down:  visualAngle = 180; break;
        case Orientation::Left:  visualAngle = 270; break;
    }
    
    for (int i = 0; i < SPAWN_COUNT; i++) {
        QPointF carPos = getCarPosition(i);
        
        painter->save();
        painter->translate(carPos);
        painter->rotate(visualAngle);
        
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
    prepareGeometryChange();
    int angle = static_cast<int>(currentOrientation);
    angle = (angle + 270) % 360;
    currentOrientation = static_cast<Orientation>(angle);
    update();
}

void SpawnFormationItem::rotateRight() {
    prepareGeometryChange();
    int angle = static_cast<int>(currentOrientation);
    angle = (angle + 90) % 360;
    currentOrientation = static_cast<Orientation>(angle);
    update();
}

std::vector<SpawnPointData> SpawnFormationItem::getSpawnPoints() const {
    std::vector<SpawnPointData> points;
    QPointF center = pos();
    
    // Ángulos en radianes
    float angle;
    switch (currentOrientation) {
        case Orientation::Up:    angle = M_PI;       break;  // arriba
        case Orientation::Right: angle = -M_PI / 2;  break;  // derecha
        case Orientation::Down:  angle = 0.0f;       break;  // abajo
        case Orientation::Left:  angle = M_PI / 2;   break;  // izquierda
        default:                 angle = M_PI;       break;
    }
    
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
    
    // Interpreto el ángulo en radianes
    float angle = spawnPoints[0].angle;
    
    // Normalizo a [-π, π]
    while (angle > M_PI) angle -= 2 * M_PI;
    while (angle < -M_PI) angle += 2 * M_PI;
    
    // Determino la orientación según el ángulo
    // π = arriba, -π/2 = derecha, 0 = abajo, π/2 = izquierda
    if (angle > -M_PI/4 && angle <= M_PI/4) {
        currentOrientation = Orientation::Down;
    } else if (angle > M_PI/4 && angle <= 3*M_PI/4) {
        currentOrientation = Orientation::Left;
    } else if (angle > 3*M_PI/4 || angle <= -3*M_PI/4) {
        currentOrientation = Orientation::Up;
    } else {
        currentOrientation = Orientation::Right;
    }
    
    update();
}

Orientation SpawnFormationItem::getOrientation() const {
    return currentOrientation;
}
