#ifndef SPAWN_FORMATION_ITEM_H
#define SPAWN_FORMATION_ITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include <vector>

enum class Orientation { Up = 0, Right = 90, Down = 180, Left = 270 };

struct SpawnPointData {
    int id;
    float x;
    float y;
    float angle;
    std::string description;
    
    SpawnPointData(int id = 0, float x = 0, float y = 0, float angle = 0)
        : id(id), x(x), y(y), angle(angle), description("Spawn " + std::to_string(id)) {}
};

class SpawnFormationItem : public QGraphicsItem {
public:
    static constexpr int SPAWN_COUNT = 8;
    
    SpawnFormationItem(QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    void setOrientation(Orientation orientation);
    Orientation getOrientation() const { return currentOrientation; }
    
    void rotateLeft();
    void rotateRight();

    std::vector<SpawnPointData> getSpawnPoints() const;
    
    void loadFromSpawnPoints(const std::vector<SpawnPointData>& spawnPoints);

private:
    Orientation currentOrientation;
    
    static constexpr float COL_SPACING = 25.0f;
    static constexpr float ROW_SPACING = 40.0f;
    static constexpr float CAR_WIDTH = 20.0f;
    static constexpr float CAR_HEIGHT = 12.0f;
    
    QPointF getCarPosition(int carIndex) const;
};

#endif
