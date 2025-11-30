#include "GameLobbyWindow.h"
#include "ui_GameLobbyWindow.h"

#include <QMessageBox>
#include <iostream>

GameLobbyWindow::GameLobbyWindow(std::shared_ptr<LobbyClient> lobby,
                                 const QString& gameName,
                                 uint32_t gameId,
                                 uint32_t playerId,
                                 bool isHost,
                                 QWidget* parent)
    : QDialog(parent)
    ,ui(new Ui::GameLobbyWindow)
    ,lobbyClient_(std::move(lobby))
    ,gameName_(gameName)
    ,gameId_(gameId)
    ,playerId_(playerId)
    ,isHost_(isHost)
    ,gameStarted_(false)
    ,forceClosed_(false)
    ,pollTimer_(nullptr)
{
    ui->setupUi(this);
    
    connect(ui->startGameButton, &QPushButton::clicked, this, &GameLobbyWindow::onStartGameClicked);
    
    updateUI();
    
    pollTimer_ = new QTimer(this);
    connect(pollTimer_, &QTimer::timeout, this, &GameLobbyWindow::onPollTimer);
    pollTimer_->start(200);
}

GameLobbyWindow::~GameLobbyWindow() {
    if (pollTimer_) {
        pollTimer_->stop();
    }
    delete ui;
}

void GameLobbyWindow::updateUI() {
    ui->gameNameLabel->setText(gameName_);
    
    ui->gameIdLabel->hide();
    ui->playerIdLabel->hide();
    
    if (isHost_) {
        setWindowTitle("Game Lobby - Host");
        ui->roleLabel->setText("You are the HOST\nPress Start when ready!");
        ui->startGameButton->setEnabled(true);
        ui->startGameButton->setText("Start Game");
    } else {
        setWindowTitle("Game Lobby");
        ui->roleLabel->setText("Waiting for host to start the race...");
        ui->startGameButton->setEnabled(false);
        ui->startGameButton->setText("Waiting...");
    }
}

void GameLobbyWindow::onStartGameClicked() {
    if (!lobbyClient_ || !lobbyClient_->isConnected()) {
        QMessageBox::warning(this, "Error", "Lost connection to server");
        reject();
        return;
    }
    
    bool success = lobbyClient_->startGame();
    
    if (success) {
        ui->startGameButton->setEnabled(false);
        ui->startGameButton->setText("Starting...");
    } else {
        QMessageBox::warning(this, "Error", "Failed to start the game");
    }
}

void GameLobbyWindow::onPollTimer() {
    if (!lobbyClient_ || !lobbyClient_->isConnected()) {
        if (pollTimer_) {
            pollTimer_->stop();
        }
        return;
    }
    
    if (lobbyClient_->checkGameStarted()) {
        gameStarted_ = true;
        
        if (pollTimer_) {
            pollTimer_->stop();
        }
        
        emit gameStartedSignal();
        accept();
    }
}

void GameLobbyWindow::closeEvent(QCloseEvent* event) {
    if (pollTimer_) {
        pollTimer_->stop();
    }
    
    forceClosed_ = true;
    
    if (lobbyClient_) {
        lobbyClient_->leaveGame();
    }
    
    event->accept();
    reject();
}
