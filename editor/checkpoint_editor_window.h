#ifndef CHECKPOINT_EDITOR_WINDOW_H
#define CHECKPOINT_EDITOR_WINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDockWidget>
#include <QWheelEvent>
#include <vector>
#include <string>

#include "checkpoint_data.h"
#include "checkpoint_item.h"

class CheckpointEditorWindow : public QMainWindow {
    Q_OBJECT

private:
    QGraphicsScene* mapScene;
    QGraphicsView* mapView;
    QGraphicsPixmapItem* backgroundItem;
    
    QDockWidget* sidePanel;
    QListWidget* checkpointList;
    QPushButton* saveButton;
    QPushButton* loadButton;
    QPushButton* deleteButton;
    QPushButton* moveUpButton;
    QPushButton* moveDownButton;
    QPushButton* zoomInButton;
    QPushButton* zoomOutButton;
    QPushButton* resetZoomButton;
    QLabel* infoLabel;
    
    std::vector<CheckpointData> checkpoints;
    std::vector<CheckpointItem*> checkpointItems;
    QGraphicsPathItem* routeLine;
    
    std::string mapImagePath;
    std::string checkpointsJsonPath;
    
    // drag del mapa
    bool isDragging;
    QPoint lastDragPos;
    
    void setupUI();
    void setupMapView();
    void setupSidePanel();
    void loadMapImage();
    void loadCheckpointsFromJSON();
    void saveCheckpointsToJSON();
    void addCheckpointItem(float x, float y);
    void removeSelectedCheckpoint();
    void updateCheckpointList();
    void updateCheckpointVisuals();
    void updateRouteLine();
    void reorderCheckpoint(int fromIndex, int toIndex);
    
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void centerOnCheckpoints();
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onMapClicked(QPointF scenePos);
    void onSaveClicked();
    void onLoadClicked();
    void onDeleteClicked();
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onCheckpointListItemChanged(QListWidgetItem* item);
    void onCheckpointItemMoved();

public:
    explicit CheckpointEditorWindow(QWidget* parent = nullptr);
    ~CheckpointEditorWindow();
};

#endif
