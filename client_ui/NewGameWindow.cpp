#include "NewGameWindow.h"
#include "ui_NewGameWindow.h"
#include "GameLobbyWindow.h"
#include "GameLauncher.h"
#include "CarSelectionDialog.h"
#include "../common/constants.h"

#include <QMessageBox>
#include <QApplication>
#include <QPixmap>
#include <iostream>

NewGameWindow::NewGameWindow(std::shared_ptr<LobbyClient> lobby, QWidget* parent)
    : QDialog(parent)
    ,ui(new Ui::NewGameWindow)
    ,lobbyClient_(std::move(lobby))
    ,gameStarted_(false)
    ,gameId_(0)
    ,playerId_(0)
    ,currentMapIndex_(0)
{
    ui->setupUi(this);
    ui->gameNameLineEdit->setText("Name...");
    ui->maxPlayersSpinBox->setMinimum(1);
    ui->maxPlayersSpinBox->setMaximum(8);
    ui->maxPlayersSpinBox->setValue(2);

    connect(ui->createButton, &QPushButton::clicked, this, &NewGameWindow::onCreate);
    connect(ui->cancelButton, &QPushButton::clicked, this, &NewGameWindow::onBack);
    connect(ui->prevMapButton, &QPushButton::clicked, this, &NewGameWindow::onPrevMap);
    connect(ui->nextMapButton, &QPushButton::clicked, this, &NewGameWindow::onNextMap);
    
    updateMapDisplay();
}

NewGameWindow::~NewGameWindow() { 
    delete ui; 
}

void NewGameWindow::onCreate() {
    if (!lobbyClient_) {
        QMessageBox::warning(this, "New Game", "There is no connection information.");
        return;
    }
    
    const QString gameName = ui->gameNameLineEdit->text().trimmed();
    const int maxPlayers = ui->maxPlayersSpinBox->value();
    (void)maxPlayers;

    if (gameName.isEmpty()) {
        QMessageBox::warning(this, "New Game", "Game name is missing.");
        return;
    }

    if (!lobbyClient_->isConnected()) {
        QMessageBox::warning(this, "Error", "Lost connection to server.");
        return;
    }
    
    uint32_t outGameId = 0;
    uint32_t outPlayerId = 0;
    uint8_t mapId = static_cast<uint8_t>(currentMapIndex_);
    bool success = lobbyClient_->createGame(gameName.toStdString(), outGameId, outPlayerId, mapId);
    
    if (!success) {
        QMessageBox::warning(this, "Error", "Failed to create game. Please try again.");
        return;
    }
    
    gameId_ = outGameId;
    playerId_ = outPlayerId;
    
    std::cout << "[NewGameWindow] Game created successfully. game_id=" << gameId_ 
              << " player_id=" << playerId_ << std::endl;
    
    this->hide();
    
    CarSelectionDialog carDialog(this);
    if (carDialog.exec() == QDialog::Accepted) {
        std::string selectedCar = carDialog.getSelectedCarType();
        lobbyClient_->selectCar(selectedCar);
    }
    
    GameLobbyWindow lobbyWindow(lobbyClient_, gameName, gameId_, playerId_, true, this);
    int result = lobbyWindow.exec();
    
    if (result == QDialog::Accepted && lobbyWindow.wasGameStarted()) {
        gameStarted_ = true;
        
        auto connection = lobbyClient_->extractConnection();
        
        std::cout << "[NewGameWindow] Transfiriendo conexión al cliente SDL" << std::endl;
        GameLauncher::launchWithConnection(std::move(connection));
        
        accept(); 
    } else if (lobbyWindow.wasForceClosed()) {
        QApplication::quit();
    } else {
        gameStarted_ = false;
        this->show();
    }
}

void NewGameWindow::onBack() { 
    reject(); 
}

void NewGameWindow::onPrevMap() {
    currentMapIndex_--;
    if (currentMapIndex_ < 0) {
        currentMapIndex_ = MAP_COUNT - 1;
    }
    updateMapDisplay();
}

void NewGameWindow::onNextMap() {
    currentMapIndex_++;
    if (currentMapIndex_ >= MAP_COUNT) {
        currentMapIndex_ = 0;
    }
    updateMapDisplay();
}

void NewGameWindow::updateMapDisplay() {
    if (currentMapIndex_ < 0 || currentMapIndex_ >= MAP_COUNT) {
        return;
    }
    
    ui->mapNameLabel->setText(QString::fromUtf8(MAP_NAMES[currentMapIndex_]));
    
    // Construir path dinámicamente como en CarSelectionDialog
    static const char* mapFiles[] = {"liberty_city", "san_andreas", "vice_city"};
    QString imagePath = QString(":/img/%1.png").arg(mapFiles[currentMapIndex_]);
    QPixmap pixmap(imagePath);
    
    if (!pixmap.isNull()) {
        pixmap = pixmap.scaled(ui->mapImageLabel->size(), 
                               Qt::KeepAspectRatio, 
                               Qt::SmoothTransformation);
        ui->mapImageLabel->setPixmap(pixmap);
    } else {
        ui->mapImageLabel->setText(QString("[ %1 ]").arg(MAP_NAMES[currentMapIndex_]));
    }
}

bool NewGameWindow::wasGameStarted() const {
    return gameStarted_;
}

uint32_t NewGameWindow::getGameId() const {
    return gameId_;
}

uint32_t NewGameWindow::getPlayerId() const {
    return playerId_;
}
