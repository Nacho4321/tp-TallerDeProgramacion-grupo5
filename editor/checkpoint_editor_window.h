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
#include <QComboBox>
#include <QGroupBox>
#include <vector>
#include <string>

#include "checkpoint_data.h"
#include "checkpoint_item.h"
#include "editor_config.h"

class CheckpointEditorWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit CheckpointEditorWindow(QWidget* parent = nullptr);
    ~CheckpointEditorWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QGraphicsScene* mapScene;
    QGraphicsView* mapView;
    QGraphicsPixmapItem* backgroundItem;
    QGraphicsPathItem* routeLine;
    QDockWidget* sidePanel;
    QComboBox* raceSelector;
    
    QListWidget* checkpointList;
    QPushButton* moveUpButton;
    QPushButton* moveDownButton;
    QPushButton* deleteButton;
    
    QPushButton* zoomInButton;
    QPushButton* zoomOutButton;
    QPushButton* resetZoomButton;

    QPushButton* saveButton;
    QPushButton* loadButton;
    
    QLabel* infoLabel;
    QLabel* currentFileLabel;

    std::vector<CheckpointData> checkpoints;
    std::vector<CheckpointItem*> checkpointItems;
    
    std::string mapImagePath;
    RaceId currentRace;
    bool hasUnsavedChanges;
    bool isDragging;
    QPoint lastDragPos;
    void setupUI();
    void setupMapView();
    void setupSidePanel();
    void loadConfiguration();
    void loadMapImage();

    void switchToRace(RaceId race);
    bool confirmDiscardChanges();
    std::string getCurrentCheckpointsPath() const;

    void loadCheckpoints();
    void saveCheckpoints();
    void clearAllCheckpoints();

    void addCheckpointAt(float x, float y);
    void removeSelectedCheckpoint();
    void swapCheckpoints(int indexA, int indexB);

    void refreshCheckpointList();
    void refreshCheckpointVisuals();
    void refreshRouteLine();
    void updateInfoLabels();
    void markAsModified();

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void centerOnCheckpoints();

private slots:
    void onRaceSelectionChanged(int index);
    void onMapClicked(QPointF scenePos);
    void onSaveClicked();
    void onLoadClicked();
    void onDeleteClicked();
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onCheckpointListItemChanged(QListWidgetItem* item);
};

#endif
