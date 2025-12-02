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
#include <QRadioButton>
#include <vector>
#include <string>

#include "checkpoint_data.h"
#include "checkpoint_item.h"
#include "spawn_formation_item.h"
#include "editor_config.h"

enum class EditorMode { Checkpoints, Spawnpoints };

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
    
    QRadioButton* checkpointModeRadio;
    QRadioButton* spawnpointModeRadio;
    EditorMode currentMode;

    QComboBox* raceSelector;
    QLabel* currentFileLabel;
    
    QGroupBox* checkpointsGroup;
    QListWidget* checkpointList;
    QPushButton* moveUpButton;
    QPushButton* moveDownButton;
    QPushButton* deleteButton;
    QLabel* infoLabel;
    
    QGroupBox* spawnpointsGroup;
    QPushButton* rotateLeftButton;
    QPushButton* rotateRightButton;
    QLabel* spawnInfoLabel;
    
    QPushButton* zoomInButton;
    QPushButton* zoomOutButton;
    QPushButton* resetZoomButton;
    
    QPushButton* saveButton;
    QPushButton* loadButton;

    std::vector<CheckpointData> checkpoints;
    std::vector<CheckpointItem*> checkpointItems;
    
    SpawnFormationItem* spawnFormation;
    
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

    void setEditorMode(EditorMode mode);
    
    void switchToRace(RaceId race);
    std::string getCurrentCheckpointsPath() const;
    std::string getSpawnPointsPath() const;

    void loadCheckpoints();
    void saveCheckpoints();
    void clearAllCheckpoints();
    void addCheckpointAt(float x, float y);
    void removeSelectedCheckpoint();
    void swapCheckpoints(int indexA, int indexB);

    void loadSpawnPoints();
    void saveSpawnPoints();
    void createDefaultSpawnFormation(float x, float y);

    void refreshCheckpointList();
    void refreshCheckpointVisuals();
    void refreshRouteLine();
    void updateInfoLabels();
    void markAsModified();

    void zoomIn();
    void zoomOut();
    void resetZoom();

private slots:
    void onModeChanged();
    void onRaceSelectionChanged(int index);
    void onMapClicked(QPointF scenePos);
    void onSaveClicked();
    void onLoadClicked();
    void onDeleteClicked();
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onRotateLeftClicked();
    void onRotateRightClicked();
};

#endif
