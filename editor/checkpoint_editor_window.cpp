#include "checkpoint_editor_window.h"
#include <QGraphicsSceneMouseEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

CheckpointEditorWindow::CheckpointEditorWindow(QWidget* parent)
    : QMainWindow(parent),
      mapScene(nullptr),
      mapView(nullptr),
      backgroundItem(nullptr),
      sidePanel(nullptr),
      checkpointList(nullptr),
      saveButton(nullptr),
      loadButton(nullptr),
      deleteButton(nullptr),
      moveUpButton(nullptr),
      moveDownButton(nullptr),
      zoomInButton(nullptr),
      zoomOutButton(nullptr),
      resetZoomButton(nullptr),
      infoLabel(nullptr),
      routeLine(nullptr),
      // por ahora hardcodeado, despues cambiar
      mapImagePath("../data/cities/Game Boy _ GBC - Grand Theft Auto - Backgrounds - Liberty City.png"),
      checkpointsJsonPath("../data/cities/base_liberty_city_checkpoints.json"),
      isDragging(false),
      lastDragPos() {
    
    setupUI();
    loadMapImage();
    loadCheckpointsFromJSON();
}

CheckpointEditorWindow::~CheckpointEditorWindow() {
}

void CheckpointEditorWindow::setupUI() {
    setWindowTitle("CheckpointEditorWindow");
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
    sidePanel = new QDockWidget("Checkpoints", this);
    sidePanel->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    
    QWidget* panelWidget = new QWidget(sidePanel);
    QVBoxLayout* layout = new QVBoxLayout(panelWidget);
    
    infoLabel = new QLabel("Click on the map to add checkpoints", panelWidget);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    QLabel* listLabel = new QLabel("Checkpoint List:", panelWidget);
    layout->addWidget(listLabel);
    
    checkpointList = new QListWidget(panelWidget);
    layout->addWidget(checkpointList);
    
    QHBoxLayout* buttonLayout1 = new QHBoxLayout();
    moveUpButton = new QPushButton("Move Up", panelWidget);
    moveDownButton = new QPushButton("Move Down", panelWidget);
    buttonLayout1->addWidget(moveUpButton);
    buttonLayout1->addWidget(moveDownButton);
    layout->addLayout(buttonLayout1);
    
    deleteButton = new QPushButton("Delete Selected", panelWidget);
    layout->addWidget(deleteButton);
    
    QFrame* line = new QFrame(panelWidget);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);
    
    QLabel* zoomLabel = new QLabel("Zoom:", panelWidget);
    layout->addWidget(zoomLabel);
    
    QHBoxLayout* zoomLayout = new QHBoxLayout();
    zoomInButton = new QPushButton("+ Zoom In", panelWidget);
    zoomOutButton = new QPushButton("- Zoom Out", panelWidget);
    zoomLayout->addWidget(zoomInButton);
    zoomLayout->addWidget(zoomOutButton);
    layout->addLayout(zoomLayout);
    
    resetZoomButton = new QPushButton("⟲ Reset Zoom", panelWidget);
    layout->addWidget(resetZoomButton);
    
    QFrame* line2 = new QFrame(panelWidget);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line2);
    
    QHBoxLayout* buttonLayout2 = new QHBoxLayout();
    loadButton = new QPushButton("Load", panelWidget);
    saveButton = new QPushButton("Save", panelWidget);
    buttonLayout2->addWidget(loadButton);
    buttonLayout2->addWidget(saveButton);
    layout->addLayout(buttonLayout2);
    
    // Información adicional
    QLabel* helpLabel = new QLabel(
        "<b>Help:</b><br>"
        "• Left Click: Add Checkpoint<br>"
        "• Right Click + Drag: Move Map<br>"
        "• Zoom: Mouse Wheel<br>"
        "• Green (S): Start<br>"
        "• Red (F): Finish<br>"
        "• Yellow: Checkpoint Intermediates<br>",
        panelWidget
    );
    helpLabel->setWordWrap(true);
    layout->addWidget(helpLabel);
    
    layout->addStretch();
    
    panelWidget->setLayout(layout);
    sidePanel->setWidget(panelWidget);
    addDockWidget(Qt::RightDockWidgetArea, sidePanel);
    
    connect(saveButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onSaveClicked);
    connect(loadButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onLoadClicked);
    connect(deleteButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onDeleteClicked);
    connect(moveUpButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onMoveUpClicked);
    connect(moveDownButton, &QPushButton::clicked, this, &CheckpointEditorWindow::onMoveDownClicked);
    connect(zoomInButton, &QPushButton::clicked, this, &CheckpointEditorWindow::zoomIn);
    connect(zoomOutButton, &QPushButton::clicked, this, &CheckpointEditorWindow::zoomOut);
    connect(resetZoomButton, &QPushButton::clicked, this, &CheckpointEditorWindow::resetZoom);
}

void CheckpointEditorWindow::loadMapImage() {
    QPixmap mapPixmap(QString::fromStdString(mapImagePath));
    
    if (mapPixmap.isNull()) {
        QMessageBox::warning(this, "Error", 
            QString("No se pudo cargar la imagen del mapa:\n%1\n\nSe usará un fondo gris.").arg(QString::fromStdString(mapImagePath)));

        // creo un placeholder gris
        mapPixmap = QPixmap(2048, 2048);
        mapPixmap.fill(QColor(100, 100, 100));
    }
    
    backgroundItem = mapScene->addPixmap(mapPixmap);
    backgroundItem->setZValue(-1);
    
    mapScene->setSceneRect(backgroundItem->boundingRect());
    
    if (!checkpoints.empty()) {
        centerOnCheckpoints();
    }
}

void CheckpointEditorWindow::loadCheckpointsFromJSON() {
    try {
        std::ifstream file(checkpointsJsonPath);
        if (!file.is_open()) {
            return;
        }
        
        json j;
        file >> j;
        file.close();
        
        if (!j.contains("checkpoints") || !j["checkpoints"].is_array()) {
            throw std::runtime_error("Json format not valid: missing 'checkpoints' array");
        }
        
        checkpoints.clear();
        for (auto* item : checkpointItems) {
            mapScene->removeItem(item);
            delete item;
        }
        checkpointItems.clear();
        
        for (const auto& cp : j["checkpoints"]) {
            int id = cp["id"];
            float x = cp["x"];
            float y = cp["y"];
            
            checkpoints.push_back(CheckpointData(id, x, y));
            addCheckpointItem(x, y);
        }
        
        updateCheckpointList();
        updateCheckpointVisuals();
        updateRouteLine();
        
        qDebug() << "Checkpoints loaded:" << checkpoints.size();
        
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error loading", 
            QString("Error loading checkpoints:\n%1").arg(e.what()));
    }
}

void CheckpointEditorWindow::saveCheckpointsToJSON() {
    try {
        json j;
        j["checkpoints"] = json::array();
        
        // sincronizo el vector de checkpoints con los items graficos
        checkpoints.clear();
        
        for (size_t i = 0; i < checkpointItems.size(); ++i) {
            QRectF rect = checkpointItems[i]->rect();
            QPointF pos = checkpointItems[i]->pos();
            
            float centerX = pos.x() + rect.x() + rect.width() / 2.0f;
            float centerY = pos.y() + rect.y() + rect.height() / 2.0f;
            
            CheckpointData cp;
            cp.id = i + 1;
            cp.x = centerX;
            cp.y = centerY;
            
            checkpoints.push_back(cp);
            
            json checkpoint;
            checkpoint["id"] = cp.id;
            checkpoint["x"] = cp.x;
            checkpoint["y"] = cp.y;
            
            j["checkpoints"].push_back(checkpoint);
        }
        
        std::ofstream file(checkpointsJsonPath);
        if (!file.is_open()) {
            throw std::runtime_error("No se pudo abrir el archivo para escritura");
        }
        
        file << j.dump(2);
        file.close();
        
        QMessageBox::information(this, "Guardado", 
            QString("Checkpoints saved successfully in:\n%1").arg(QString::fromStdString(checkpointsJsonPath)));

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error saving", 
            QString("Error saving checkpoints:\n%1").arg(e.what()));
    }
}

void CheckpointEditorWindow::addCheckpointItem(float x, float y) {
    int index = checkpointItems.size();
    CheckpointItem* item = new CheckpointItem(x, y, index);
    
    mapScene->addItem(item);
    checkpointItems.push_back(item);
    
    // vector de datos
    CheckpointData cp;
    cp.id = index + 1;
    cp.x = x;
    cp.y = y;
    checkpoints.push_back(cp);
    
    updateCheckpointList();
    updateCheckpointVisuals();
    updateRouteLine();
}

void CheckpointEditorWindow::removeSelectedCheckpoint() {
    int currentRow = checkpointList->currentRow();
    
    if (currentRow < 0 || currentRow >= static_cast<int>(checkpointItems.size())) {
        QMessageBox::warning(this, "Delete", "Select a checkpoint to delete.");
        return;
    }
    
    mapScene->removeItem(checkpointItems[currentRow]);
    delete checkpointItems[currentRow];
    checkpointItems.erase(checkpointItems.begin() + currentRow);
    
    checkpoints.erase(checkpoints.begin() + currentRow);
    
    // reindexar items restantes
    for (size_t i = 0; i < checkpointItems.size(); ++i) {
        checkpointItems[i]->setIndex(i);
    }
    
    updateCheckpointList();
    updateCheckpointVisuals();
    updateRouteLine();
}

void CheckpointEditorWindow::updateCheckpointList() {
    if (!checkpointList || !infoLabel) {
        return;
    }
    
    checkpointList->clear();
    
    for (size_t i = 0; i < checkpoints.size(); ++i) {
        QString label;
        if (i == 0) {
            label = QString("START (%1): x=%2, y=%3")
                .arg(i + 1)
                .arg(checkpoints[i].x, 0, 'f', 1)
                .arg(checkpoints[i].y, 0, 'f', 1);
        } else if (i == checkpoints.size() - 1) {
            label = QString("FINISH (%1): x=%2, y=%3")
                .arg(i + 1)
                .arg(checkpoints[i].x, 0, 'f', 1)
                .arg(checkpoints[i].y, 0, 'f', 1);
        } else {
            label = QString("Checkpoint %1: x=%2, y=%3")
                .arg(i + 1)
                .arg(checkpoints[i].x, 0, 'f', 1)
                .arg(checkpoints[i].y, 0, 'f', 1);
        }
        checkpointList->addItem(label);
    }
    
    infoLabel->setText(QString("Total de checkpoints: %1").arg(checkpoints.size()));
}

void CheckpointEditorWindow::updateCheckpointVisuals() {
    for (size_t i = 0; i < checkpointItems.size(); ++i) {
        bool isStart = (i == 0);
        bool isFinish = (i == checkpointItems.size() - 1);
        
        checkpointItems[i]->setIsStart(isStart);
        checkpointItems[i]->setIsFinish(isFinish);
    }
}

void CheckpointEditorWindow::updateRouteLine() {
    if (!mapScene) {
        return; 
    }
    
    if (routeLine) {
        mapScene->removeItem(routeLine);
        delete routeLine;
        routeLine = nullptr;
    }
    

    if (checkpointItems.size() < 2) {
        return;
    }

    // creo una linea que conecte los checkpoints
    QPainterPath path;
    bool first = true;
    
    for (auto* item : checkpointItems) {
        QRectF rect = item->rect();
        QPointF pos = item->pos();
        QPointF center(pos.x() + rect.x() + rect.width() / 2.0f,
                      pos.y() + rect.y() + rect.height() / 2.0f);
        
        if (first) {
            path.moveTo(center);
            first = false;
        } else {
            path.lineTo(center);
        }
    }
    
    routeLine = mapScene->addPath(path, QPen(QColor(255, 255, 255, 100), 2, Qt::DashLine));
    routeLine->setZValue(-0.5);
}

void CheckpointEditorWindow::reorderCheckpoint(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= static_cast<int>(checkpointItems.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(checkpointItems.size())) {
        return;
    }
    
    std::swap(checkpointItems[fromIndex], checkpointItems[toIndex]);
    
    std::swap(checkpoints[fromIndex], checkpoints[toIndex]);
    
    checkpointItems[fromIndex]->setIndex(fromIndex);
    checkpointItems[toIndex]->setIndex(toIndex);
    
    updateCheckpointList();
    updateCheckpointVisuals();
    updateRouteLine();
}

bool CheckpointEditorWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == mapView->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            // click derecho: drag del mapa
            if (mouseEvent->button() == Qt::RightButton) {
                isDragging = true;
                lastDragPos = mouseEvent->pos();
                mapView->setCursor(Qt::ClosedHandCursor);
                return true;
            }
            
            // click izquierdo: agregar checkpoint
            if (mouseEvent->button() == Qt::LeftButton) {
                QPointF scenePos = mapView->mapToScene(mouseEvent->pos());
                
                // verificar que no se hizo clic sobre un item existente
                QGraphicsItem* itemAtPos = mapScene->itemAt(scenePos, mapView->transform());
                if (!itemAtPos || itemAtPos == backgroundItem || itemAtPos == routeLine) {
                    onMapClicked(scenePos);
                    return true;
                }
            }
        }
        else if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            
            // Si estamos arrastrando con click derecho
            if (isDragging) {
                QPoint delta = mouseEvent->pos() - lastDragPos;
                lastDragPos = mouseEvent->pos();
                
                // Mover las barras de scroll
                mapView->horizontalScrollBar()->setValue(
                    mapView->horizontalScrollBar()->value() - delta.x());
                mapView->verticalScrollBar()->setValue(
                    mapView->verticalScrollBar()->value() - delta.y());
                
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            
            // Soltar click derecho: terminar drag
            if (mouseEvent->button() == Qt::RightButton && isDragging) {
                isDragging = false;
                mapView->setCursor(Qt::ArrowCursor);
                return true;
            }
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}

void CheckpointEditorWindow::onMapClicked(QPointF scenePos) {
    addCheckpointItem(scenePos.x(), scenePos.y());
}

void CheckpointEditorWindow::onSaveClicked() {
    saveCheckpointsToJSON();
}

void CheckpointEditorWindow::onLoadClicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Cargar",
        "Reload checkpoints from file?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        loadCheckpointsFromJSON();
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
    
    reorderCheckpoint(currentRow, currentRow - 1);
    checkpointList->setCurrentRow(currentRow - 1);
}

void CheckpointEditorWindow::onMoveDownClicked() {
    int currentRow = checkpointList->currentRow();
    
    if (currentRow < 0 || currentRow >= static_cast<int>(checkpointItems.size()) - 1) {
        return; 
    }
    
    reorderCheckpoint(currentRow, currentRow + 1);
    checkpointList->setCurrentRow(currentRow + 1);
}

void CheckpointEditorWindow::onCheckpointListItemChanged(QListWidgetItem* item) {
    Q_UNUSED(item);
    // implementar si hace falta
}

void CheckpointEditorWindow::onCheckpointItemMoved() {
    updateRouteLine();
    updateCheckpointList();
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

void CheckpointEditorWindow::centerOnCheckpoints() {
    if (checkpoints.empty()) {
        return;
    }
    
    // Calcular el rectángulo que contiene todos los checkpoints
    float minX = checkpoints[0].x;
    float maxX = checkpoints[0].x;
    float minY = checkpoints[0].y;
    float maxY = checkpoints[0].y;
    
    for (const auto& cp : checkpoints) {
        minX = std::min(minX, cp.x);
        maxX = std::max(maxX, cp.x);
        minY = std::min(minY, cp.y);
        maxY = std::max(maxY, cp.y);
    }
    
    float margin = 200.0f;
    QRectF rect(minX - margin, minY - margin, 
                (maxX - minX) + 2 * margin, 
                (maxY - minY) + 2 * margin);
    
    mapView->fitInView(rect, Qt::KeepAspectRatio);
}

void CheckpointEditorWindow::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) {
        zoomIn();
    } else if (event->angleDelta().y() < 0) {
        zoomOut();
    }
    event->accept(); 
}
