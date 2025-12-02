#include "checkpoint_editor_window.h"
#include "install_paths.h"
#include <QGraphicsSceneMouseEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QRadioButton>
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

CheckpointEditorWindow::CheckpointEditorWindow(QWidget* parent)
    : QMainWindow(parent),
      mapScene(nullptr),
      mapView(nullptr),
      backgroundItem(nullptr),
      routeLine(nullptr),
      sidePanel(nullptr),
      checkpointModeRadio(nullptr),
      spawnpointModeRadio(nullptr),
      currentMode(EditorMode::Checkpoints),
      raceSelector(nullptr),
      currentFileLabel(nullptr),
      checkpointsGroup(nullptr),
      checkpointList(nullptr),
      moveUpButton(nullptr),
      moveDownButton(nullptr),
      deleteButton(nullptr),
      infoLabel(nullptr),
      spawnpointsGroup(nullptr),
      rotateLeftButton(nullptr),
      rotateRightButton(nullptr),
      spawnInfoLabel(nullptr),
      zoomInButton(nullptr),
      zoomOutButton(nullptr),
      resetZoomButton(nullptr),
      saveButton(nullptr),
      loadButton(nullptr),
      spawnFormation(nullptr),
      currentRace(RaceId::Race1),
      hasUnsavedChanges(false),
      isDragging(false),
      lastDragPos() {
    
    loadConfiguration();
    setupUI();
    loadMapImage();
    loadCheckpoints();
    loadSpawnPoints();
}

CheckpointEditorWindow::~CheckpointEditorWindow() {}

void CheckpointEditorWindow::loadConfiguration() {
    EditorConfig& config = EditorConfig::getInstance();
    
    if (!config.loadFromFile("")) {
        QMessageBox::warning(this, "Configuración",
            "No se pudo cargar la configuración del editor.\n"
            "Se usarán valores por defecto.");
        mapImagePath = std::string(DATA_DIR) + "/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Liberty City.png";
    } else {
        mapImagePath = config.getMapImagePath();
    }
}

void CheckpointEditorWindow::setupUI() {
    setWindowTitle("Checkpoint Editor - Need for Speed");
    resize(1400, 900);
    
    setupMapView();
    setupSidePanel();
}

void CheckpointEditorWindow::setupMapView() {
    mapScene = new QGraphicsScene(this);
    mapView = new QGraphicsView(mapScene, this);
    
    mapView->setRenderHint(QPainter::Antialiasing);
    mapView->setDragMode(QGraphicsView::NoDrag);
    mapView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mapView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mapView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    
    setCentralWidget(mapView);
    mapView->viewport()->installEventFilter(this);
}

void CheckpointEditorWindow::setupSidePanel() {
    sidePanel = new QDockWidget("Editor Panel", this);
    sidePanel->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    sidePanel->setMinimumWidth(280);
    
    QWidget* panelWidget = new QWidget(sidePanel);
    QVBoxLayout* mainLayout = new QVBoxLayout(panelWidget);
    mainLayout->setSpacing(10);

    QGroupBox* modeGroup = new QGroupBox("Edit Mode", panelWidget);
    QVBoxLayout* modeLayout = new QVBoxLayout(modeGroup);
    checkpointModeRadio = new QRadioButton("Checkpoints", modeGroup);
    spawnpointModeRadio = new QRadioButton("Spawnpoints", modeGroup);
    checkpointModeRadio->setChecked(true);
    modeLayout->addWidget(checkpointModeRadio);
    modeLayout->addWidget(spawnpointModeRadio);
    mainLayout->addWidget(modeGroup);

    QGroupBox* raceGroup = new QGroupBox("Race Selection", panelWidget);
    QVBoxLayout* raceLayout = new QVBoxLayout(raceGroup);
    
    raceSelector = new QComboBox(raceGroup);
    raceSelector->addItem("Race 1", static_cast<int>(RaceId::Race1));
    raceSelector->addItem("Race 2", static_cast<int>(RaceId::Race2));
    raceSelector->addItem("Race 3", static_cast<int>(RaceId::Race3));
    raceLayout->addWidget(raceSelector);
    
    currentFileLabel = new QLabel(raceGroup);
    currentFileLabel->setWordWrap(true);
    currentFileLabel->setStyleSheet("color: gray; font-size: 10px;");
    raceLayout->addWidget(currentFileLabel);
    
    mainLayout->addWidget(raceGroup);

    checkpointsGroup = new QGroupBox("Checkpoints", panelWidget);
    QVBoxLayout* checkpointsLayout = new QVBoxLayout(checkpointsGroup);
    
    infoLabel = new QLabel("Click on the map to add checkpoints", checkpointsGroup);
    infoLabel->setWordWrap(true);
    checkpointsLayout->addWidget(infoLabel);
    
    checkpointList = new QListWidget(checkpointsGroup);
    checkpointList->setMinimumHeight(150);
    checkpointsLayout->addWidget(checkpointList);
    
    QHBoxLayout* reorderLayout = new QHBoxLayout();
    moveUpButton = new QPushButton("Move Up", checkpointsGroup);
    moveDownButton = new QPushButton("Move Down", checkpointsGroup);
    reorderLayout->addWidget(moveUpButton);
    reorderLayout->addWidget(moveDownButton);
    checkpointsLayout->addLayout(reorderLayout);
    
    deleteButton = new QPushButton("Delete Selected", checkpointsGroup);
    checkpointsLayout->addWidget(deleteButton);
    
    mainLayout->addWidget(checkpointsGroup);

    spawnpointsGroup = new QGroupBox("Spawnpoints", panelWidget);
    QVBoxLayout* spawnLayout = new QVBoxLayout(spawnpointsGroup);
    
    spawnInfoLabel = new QLabel("Drag to move, buttons to rotate", spawnpointsGroup);
    spawnInfoLabel->setWordWrap(true);
    spawnLayout->addWidget(spawnInfoLabel);
    
    QHBoxLayout* rotateLayout = new QHBoxLayout();
    rotateLeftButton = new QPushButton("Rotate Left", spawnpointsGroup);
    rotateRightButton = new QPushButton("Rotate Right", spawnpointsGroup);
    rotateLayout->addWidget(rotateLeftButton);
    rotateLayout->addWidget(rotateRightButton);
    spawnLayout->addLayout(rotateLayout);
    
    spawnpointsGroup->setVisible(false);
    mainLayout->addWidget(spawnpointsGroup);

    QGroupBox* zoomGroup = new QGroupBox("Zoom", panelWidget);
    QVBoxLayout* zoomLayout = new QVBoxLayout(zoomGroup);
    
    QHBoxLayout* zoomButtonsLayout = new QHBoxLayout();
    zoomInButton = new QPushButton("+ Zoom In", zoomGroup);
    zoomOutButton = new QPushButton("- Zoom Out", zoomGroup);
    zoomButtonsLayout->addWidget(zoomInButton);
    zoomButtonsLayout->addWidget(zoomOutButton);
    zoomLayout->addLayout(zoomButtonsLayout);
    
    resetZoomButton = new QPushButton("Reset Zoom", zoomGroup);
    zoomLayout->addWidget(resetZoomButton);
    
    mainLayout->addWidget(zoomGroup);

    QGroupBox* fileGroup = new QGroupBox("File Operations", panelWidget);
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    
    QHBoxLayout* fileButtonsLayout = new QHBoxLayout();
    loadButton = new QPushButton("Load", fileGroup);
    saveButton = new QPushButton("Save", fileGroup);
    fileButtonsLayout->addWidget(loadButton);
    fileButtonsLayout->addWidget(saveButton);
    fileLayout->addLayout(fileButtonsLayout);
    
    mainLayout->addWidget(fileGroup);

    QGroupBox* helpGroup = new QGroupBox("Help", panelWidget);
    QVBoxLayout* helpLayout = new QVBoxLayout(helpGroup);
    
    QLabel* helpLabel = new QLabel(
        "• <b>Left Click:</b> Add checkpoint<br>"
        "• <b>Right Click + Drag:</b> Move map<br>"
        "• <b>Green (S):</b> Start<br>"
        "• <b>Red (F):</b> Finish",
        helpGroup
    );
    helpLabel->setWordWrap(true);
    helpLayout->addWidget(helpLabel);
    
    mainLayout->addWidget(helpGroup);

    mainLayout->addStretch();
    
    panelWidget->setLayout(mainLayout);
    sidePanel->setWidget(panelWidget);
    addDockWidget(Qt::RightDockWidgetArea, sidePanel);

    connect(checkpointModeRadio, &QRadioButton::toggled, this, &CheckpointEditorWindow::onModeChanged);
    connect(raceSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CheckpointEditorWindow::onRaceSelectionChanged);
    connect(saveButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onSaveClicked);
    connect(loadButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onLoadClicked);
    connect(deleteButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onDeleteClicked);
    connect(moveUpButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onMoveUpClicked);
    connect(moveDownButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onMoveDownClicked);
    connect(zoomInButton, &QPushButton::clicked, this, &CheckpointEditorWindow::zoomIn);
    connect(zoomOutButton, &QPushButton::clicked, this, &CheckpointEditorWindow::zoomOut);
    connect(resetZoomButton, &QPushButton::clicked, this, &CheckpointEditorWindow::resetZoom);
    connect(rotateLeftButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onRotateLeftClicked);
    connect(rotateRightButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onRotateRightClicked);
    
    updateInfoLabels();
}

void CheckpointEditorWindow::loadMapImage() {
    QPixmap mapPixmap(QString::fromStdString(mapImagePath));
    
    if (mapPixmap.isNull()) {
        QMessageBox::warning(this, "Error", 
            QString("No se pudo cargar la imagen del mapa:\n%1\n\nSe usará un fondo gris.")
                .arg(QString::fromStdString(mapImagePath)));
        
        mapPixmap = QPixmap(2048, 2048);
        mapPixmap.fill(QColor(100, 100, 100));
    }
    
    backgroundItem = mapScene->addPixmap(mapPixmap);
    backgroundItem->setZValue(-1);
    mapScene->setSceneRect(backgroundItem->boundingRect());
}

void CheckpointEditorWindow::switchToRace(RaceId race) {
    if (race == currentRace) return;
    currentRace = race;
    loadCheckpoints();
    updateInfoLabels();
}

std::string CheckpointEditorWindow::getCurrentCheckpointsPath() const {
    return EditorConfig::getInstance().getCheckpointsFilePath(currentRace);
}

void CheckpointEditorWindow::loadCheckpoints() {
    std::string filePath = getCurrentCheckpointsPath();
    
    if (filePath.empty()) {
        QMessageBox::warning(this, "Error", 
            QString("No checkpoint file configured for %1")
                .arg(QString::fromStdString(EditorConfig::getRaceName(currentRace))));
        return;
    }
    
    clearAllCheckpoints();
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            qDebug() << "File not found, starting with empty race:" << QString::fromStdString(filePath);
            hasUnsavedChanges = false;
            updateInfoLabels();
            return;
        }
        
        json j;
        file >> j;
        file.close();
        
        if (!j.contains("checkpoints") || !j["checkpoints"].is_array()) {
            throw std::runtime_error("Invalid JSON format: missing 'checkpoints' array");
        }
        
        for (const auto& cp : j["checkpoints"]) {
            int id = cp["id"];
            float x = cp["x"];
            float y = cp["y"];
            
            checkpoints.emplace_back(id, x, y);
            
            int index = static_cast<int>(checkpointItems.size());
            CheckpointItem* item = new CheckpointItem(x, y, index);
            mapScene->addItem(item);
            checkpointItems.push_back(item);
        }
        
        refreshCheckpointList();
        refreshCheckpointVisuals();
        refreshRouteLine();
        
        hasUnsavedChanges = false;
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error Loading", 
            QString("Error loading checkpoints:\n%1").arg(e.what()));
    }
    
    updateInfoLabels();
}

void CheckpointEditorWindow::saveCheckpoints() {
    std::string filePath = getCurrentCheckpointsPath();
    
    if (filePath.empty()) {
        QMessageBox::critical(this, "Error", "No checkpoint file configured for this race.");
        return;
    }
    
    try {
        json j;
        j["checkpoints"] = json::array();
        
        checkpoints.clear();
        
        for (size_t i = 0; i < checkpointItems.size(); ++i) {
            QRectF rect = checkpointItems[i]->rect();
            QPointF pos = checkpointItems[i]->pos();
            
            float centerX = pos.x() + rect.x() + rect.width() / 2.0f;
            float centerY = pos.y() + rect.y() + rect.height() / 2.0f;
            
            CheckpointData cp(static_cast<int>(i + 1), centerX, centerY);
            checkpoints.push_back(cp);
            
            json checkpoint;
            checkpoint["id"] = cp.id;
            checkpoint["x"] = cp.x;
            checkpoint["y"] = cp.y;
            j["checkpoints"].push_back(checkpoint);
        }
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file for writing");
        }
        
        file << j.dump(2);
        file.close();
        
        hasUnsavedChanges = false;
        updateInfoLabels();
        
        QMessageBox::information(this, "Saved", 
            QString("Checkpoints saved successfully to:\n%1").arg(QString::fromStdString(filePath)));
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error Saving", 
            QString("Error saving checkpoints:\n%1").arg(e.what()));
    }
}

void CheckpointEditorWindow::clearAllCheckpoints() {
    for (auto* item : checkpointItems) {
        mapScene->removeItem(item);
        delete item;
    }
    checkpointItems.clear();
    checkpoints.clear();
    
    if (routeLine) {
        mapScene->removeItem(routeLine);
        delete routeLine;
        routeLine = nullptr;
    }
    
    refreshCheckpointList();
}

void CheckpointEditorWindow::addCheckpointAt(float x, float y) {
    int index = static_cast<int>(checkpointItems.size());
    
    CheckpointItem* item = new CheckpointItem(x, y, index);
    mapScene->addItem(item);
    checkpointItems.push_back(item);
    
    CheckpointData cp(index + 1, x, y);
    checkpoints.push_back(cp);
    
    refreshCheckpointList();
    refreshCheckpointVisuals();
    refreshRouteLine();
    markAsModified();
}

void CheckpointEditorWindow::removeSelectedCheckpoint() {
    int currentRow = checkpointList->currentRow();
    
    if (currentRow < 0 || currentRow >= static_cast<int>(checkpointItems.size())) {
        return;
    }
    
    mapScene->removeItem(checkpointItems[currentRow]);
    delete checkpointItems[currentRow];
    checkpointItems.erase(checkpointItems.begin() + currentRow);
    checkpoints.erase(checkpoints.begin() + currentRow);
    
    for (size_t i = 0; i < checkpointItems.size(); ++i) {
        checkpointItems[i]->setIndex(static_cast<int>(i));
    }
    
    refreshCheckpointList();
    refreshCheckpointVisuals();
    refreshRouteLine();
    markAsModified();
}

void CheckpointEditorWindow::swapCheckpoints(int indexA, int indexB) {
    if (indexA < 0 || indexA >= static_cast<int>(checkpointItems.size()) ||
        indexB < 0 || indexB >= static_cast<int>(checkpointItems.size())) {
        return;
    }
    
    std::swap(checkpointItems[indexA], checkpointItems[indexB]);
    std::swap(checkpoints[indexA], checkpoints[indexB]);
    
    checkpointItems[indexA]->setIndex(indexA);
    checkpointItems[indexB]->setIndex(indexB);
    
    refreshCheckpointList();
    refreshCheckpointVisuals();
    refreshRouteLine();
    markAsModified();
}

void CheckpointEditorWindow::refreshCheckpointList() {
    if (!checkpointList) return;
    
    checkpointList->clear();
    
    for (size_t i = 0; i < checkpoints.size(); ++i) {
        QString label;
        
        if (i == 0) {
            label = QString("START: x=%1, y=%2")
                .arg(checkpoints[i].x, 0, 'f', 1)
                .arg(checkpoints[i].y, 0, 'f', 1);
        } else if (i == checkpoints.size() - 1 && checkpoints.size() > 1) {
            label = QString("FINISH: x=%1, y=%2")
                .arg(checkpoints[i].x, 0, 'f', 1)
                .arg(checkpoints[i].y, 0, 'f', 1);
        } else {
            label = QString("CP %1: x=%2, y=%3")
                .arg(i + 1)
                .arg(checkpoints[i].x, 0, 'f', 1)
                .arg(checkpoints[i].y, 0, 'f', 1);
        }
        
        checkpointList->addItem(label);
    }
}

void CheckpointEditorWindow::refreshCheckpointVisuals() {
    for (size_t i = 0; i < checkpointItems.size(); ++i) {
        bool isStart = (i == 0);
        bool isFinish = (i == checkpointItems.size() - 1) && (checkpointItems.size() > 1);
        
        checkpointItems[i]->setIsStart(isStart);
        checkpointItems[i]->setIsFinish(isFinish);
    }
}

void CheckpointEditorWindow::refreshRouteLine() {
    if (!mapScene) return;
    
    if (routeLine) {
        mapScene->removeItem(routeLine);
        delete routeLine;
        routeLine = nullptr;
    }
    
    if (checkpointItems.size() < 2) return;
    
    QPainterPath path;
    bool first = true;
    
    for (auto* item : checkpointItems) {
        QRectF rect = item->rect();
        QPointF pos = item->pos();
        QPointF center(
            pos.x() + rect.x() + rect.width() / 2.0f,
            pos.y() + rect.y() + rect.height() / 2.0f
        );
        
        if (first) {
            path.moveTo(center);
            first = false;
        } else {
            path.lineTo(center);
        }
    }
    
    routeLine = mapScene->addPath(path, QPen(QColor(255, 255, 255, 150), 3, Qt::DashLine));
    routeLine->setZValue(-0.5);
}

void CheckpointEditorWindow::updateInfoLabels() {
    if (infoLabel) {
        infoLabel->setText(QString("Total checkpoints: %1").arg(checkpoints.size()));
    }
    
    if (currentFileLabel) {
        std::string path = getCurrentCheckpointsPath();
        currentFileLabel->setText(QString("File: %1").arg(QString::fromStdString(path)));
    }
    
    setWindowTitle(QString("Checkpoint Editor - %1")
        .arg(QString::fromStdString(EditorConfig::getRaceName(currentRace))));
}

void CheckpointEditorWindow::markAsModified() {
    hasUnsavedChanges = true;
}

void CheckpointEditorWindow::zoomIn() {
    mapView->scale(1.25, 1.25);
}

void CheckpointEditorWindow::zoomOut() {
    mapView->scale(0.8, 0.8);
}

void CheckpointEditorWindow::resetZoom() {
    mapView->resetTransform();
}

bool CheckpointEditorWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj != mapView->viewport()) {
        return QMainWindow::eventFilter(obj, event);
    }
    
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        
        if (mouseEvent->button() == Qt::RightButton) {
            isDragging = true;
            lastDragPos = mouseEvent->pos();
            mapView->setCursor(Qt::ClosedHandCursor);
            return true;
        }
        
        if (mouseEvent->button() == Qt::LeftButton) {
            QPointF scenePos = mapView->mapToScene(mouseEvent->pos());
            QGraphicsItem* itemAtPos = mapScene->itemAt(scenePos, mapView->transform());
            
            if (!itemAtPos || itemAtPos == backgroundItem || itemAtPos == routeLine) {
                onMapClicked(scenePos);
                return true;
            }
        }
    }
    else if (event->type() == QEvent::MouseMove && isDragging) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint delta = mouseEvent->pos() - lastDragPos;
        lastDragPos = mouseEvent->pos();
        
        mapView->horizontalScrollBar()->setValue(
            mapView->horizontalScrollBar()->value() - delta.x());
        mapView->verticalScrollBar()->setValue(
            mapView->verticalScrollBar()->value() - delta.y());
        
        return true;
    }
    else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        
        if (mouseEvent->button() == Qt::RightButton && isDragging) {
            isDragging = false;
            mapView->setCursor(Qt::ArrowCursor);
            return true;
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}

void CheckpointEditorWindow::wheelEvent(QWheelEvent* event) {
    QMainWindow::wheelEvent(event);
}

void CheckpointEditorWindow::closeEvent(QCloseEvent* event) {
    event->accept();
}

void CheckpointEditorWindow::onRaceSelectionChanged(int index) {
    RaceId newRace = static_cast<RaceId>(raceSelector->itemData(index).toInt());
    switchToRace(newRace);
}

void CheckpointEditorWindow::onMapClicked(QPointF scenePos) {
    if (currentMode == EditorMode::Checkpoints) {
        addCheckpointAt(scenePos.x(), scenePos.y());
    }
}

void CheckpointEditorWindow::onSaveClicked() {
    if (currentMode == EditorMode::Checkpoints) {
        saveCheckpoints();
    } else {
        saveSpawnPoints();
    }
}

void CheckpointEditorWindow::onLoadClicked() {
    if (currentMode == EditorMode::Checkpoints) {
        loadCheckpoints();
    } else {
        loadSpawnPoints();
    }
}

void CheckpointEditorWindow::onDeleteClicked() {
    removeSelectedCheckpoint();
}

void CheckpointEditorWindow::onMoveUpClicked() {
    int currentRow = checkpointList->currentRow();

    if (currentRow <= 0) {
        return;
    }

    swapCheckpoints(currentRow, currentRow - 1);
    checkpointList->setCurrentRow(currentRow - 1);
}

void CheckpointEditorWindow::onMoveDownClicked() {
    int currentRow = checkpointList->currentRow();
    
    if (currentRow < 0 || currentRow >= static_cast<int>(checkpointItems.size()) - 1) {
        return;
    }
    
    swapCheckpoints(currentRow, currentRow + 1);
    checkpointList->setCurrentRow(currentRow + 1);
}

void CheckpointEditorWindow::setEditorMode(EditorMode mode) {
    currentMode = mode;
    
    bool isCheckpointMode = (mode == EditorMode::Checkpoints);
    checkpointsGroup->setVisible(isCheckpointMode);
    spawnpointsGroup->setVisible(!isCheckpointMode);
    
    if (spawnFormation) {
        spawnFormation->setFlag(QGraphicsItem::ItemIsMovable, !isCheckpointMode);
    }
}

void CheckpointEditorWindow::onModeChanged() {
    if (checkpointModeRadio->isChecked()) {
        setEditorMode(EditorMode::Checkpoints);
    } else {
        setEditorMode(EditorMode::Spawnpoints);
    }
}

std::string CheckpointEditorWindow::getSpawnPointsPath() const {
    return EditorConfig::getInstance().getSpawnPointsFilePath();
}

void CheckpointEditorWindow::loadSpawnPoints() {
    std::string filePath = getSpawnPointsPath();
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            createDefaultSpawnFormation(1000, 700);
            return;
        }
        
        json j;
        file >> j;
        file.close();
        
        if (!j.contains("spawn_points") || !j["spawn_points"].is_array()) {
            createDefaultSpawnFormation(1000, 700);
            return;
        }
        
        std::vector<SpawnPointData> points;
        for (const auto& sp : j["spawn_points"]) {
            SpawnPointData data;
            data.id = sp["id"];
            data.x = sp["x"];
            data.y = sp["y"];
            data.angle = sp.value("angle", 0.0f);
            data.description = sp.value("description", "Spawn " + std::to_string(data.id));
            points.push_back(data);
        }
        
        if (spawnFormation) {
            mapScene->removeItem(spawnFormation);
            delete spawnFormation;
        }
        
        spawnFormation = new SpawnFormationItem();
        spawnFormation->loadFromSpawnPoints(points);
        spawnFormation->setFlag(QGraphicsItem::ItemIsMovable, currentMode == EditorMode::Spawnpoints);
        mapScene->addItem(spawnFormation);
        
    } catch (const std::exception& e) {
        qDebug() << "Error loading spawn points:" << e.what();
        createDefaultSpawnFormation(1000, 700);
    }
}

void CheckpointEditorWindow::saveSpawnPoints() {
    if (!spawnFormation) {
        QMessageBox::warning(this, "Error", "No spawn formation to save.");
        return;
    }
    
    std::string filePath = getSpawnPointsPath();
    
    try {
        json j;
        j["spawn_points"] = json::array();
        
        auto points = spawnFormation->getSpawnPoints();
        for (const auto& sp : points) {
            json point;
            point["id"] = sp.id;
            point["x"] = sp.x;
            point["y"] = sp.y;
            point["angle"] = sp.angle;
            point["description"] = sp.description;
            j["spawn_points"].push_back(point);
        }
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file for writing");
        }
        
        file << j.dump(2);
        file.close();
        
        QMessageBox::information(this, "Saved", 
            QString("Spawn points saved to:\n%1").arg(QString::fromStdString(filePath)));
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", 
            QString("Error saving spawn points:\n%1").arg(e.what()));
    }
}

void CheckpointEditorWindow::createDefaultSpawnFormation(float x, float y) {
    if (spawnFormation) {
        mapScene->removeItem(spawnFormation);
        delete spawnFormation;
    }
    
    spawnFormation = new SpawnFormationItem();
    spawnFormation->setPos(x, y);
    spawnFormation->setFlag(QGraphicsItem::ItemIsMovable, currentMode == EditorMode::Spawnpoints);
    mapScene->addItem(spawnFormation);
}

void CheckpointEditorWindow::onRotateLeftClicked() {
    if (spawnFormation) {
        spawnFormation->rotateLeft();
    }
}

void CheckpointEditorWindow::onRotateRightClicked() {
    if (spawnFormation) {
        spawnFormation->rotateRight();
    }
}
