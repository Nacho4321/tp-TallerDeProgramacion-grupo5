#include "JoinGameWindow.h"
#include "ui_JoinGameWindow.h"
#include "GameLobbyWindow.h"
#include "GameLauncher.h"
#include "CarSelectionDialog.h"

#include <QMessageBox>
#include <iostream>

JoinGameWindow::JoinGameWindow(std::shared_ptr<LobbyClient> lobby, QWidget* parent)
    : QDialog(parent), 
      ui(new Ui::JoinGameWindow), 
      lobbyClient_(std::move(lobby)),
      selectedGameId_(-1),
      selectedGameName_(""),
      playerId_(0),
      gameStarted_(false)
{
    ui->setupUi(this);
    
    connect(ui->refreshButton, &QPushButton::clicked, this, &JoinGameWindow::onRefresh);
    connect(ui->joinButton, &QPushButton::clicked, this, &JoinGameWindow::onJoin);
    connect(ui->cancelButton, &QPushButton::clicked, this, &JoinGameWindow::onCancel);
    connect(ui->gamesListWidget, &QListWidget::itemClicked, this, &JoinGameWindow::onGameSelected);
    connect(ui->gamesListWidget, &QListWidget::itemDoubleClicked, 
            [this](QListWidgetItem*) { onJoin(); });
    
    ui->joinButton->setEnabled(false);
    
    loadGamesList();
}

JoinGameWindow::~JoinGameWindow() {
    delete ui;
}

void JoinGameWindow::loadGamesList() {
    ui->gamesListWidget->clear();
    selectedGameId_ = -1;
    selectedGameName_ = "";
    updateJoinButtonState();
    
    if (!lobbyClient_ || !lobbyClient_->isConnected()) {
        QMessageBox::warning(this, "Error", "No server connection");
        return;
    }
    
    try {
        auto games = lobbyClient_->listGames();

        if (games.empty()) {
            QListWidgetItem* item = new QListWidgetItem("No games available");
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            item->setData(Qt::UserRole, -1);
            item->setData(Qt::UserRole + 1, "");
            ui->gamesListWidget->addItem(item);

        } else {
            for (const auto& game : games) {
                QString itemText = QString("🏎️  %1  |  ID: %2  |  👥 %3 player(s)")
                    .arg(QString::fromStdString(game.name))
                    .arg(game.game_id)
                    .arg(game.player_count);
                
                QListWidgetItem* item = new QListWidgetItem(itemText);
                item->setData(Qt::UserRole, static_cast<int>(game.game_id));
                item->setData(Qt::UserRole + 1, QString::fromStdString(game.name));
                ui->gamesListWidget->addItem(item);
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[JoinGameWindow] Error loading games: " << e.what() << std::endl;
        QMessageBox::critical(this, "Error", 
            QString("Failed to load games list: %1").arg(e.what()));
    }
}

void JoinGameWindow::onRefresh() {
    loadGamesList();
}

void JoinGameWindow::onJoin() {
    if (selectedGameId_ < 0) {
        QMessageBox::warning(this, "Join Game", "Please select a game first");
        return;
    }
    
    if (!lobbyClient_ || !lobbyClient_->isConnected()) {
        QMessageBox::warning(this, "Error", "Lost connection to server");
        return;
    }
    
    uint32_t outPlayerId = 0;
    bool success = lobbyClient_->joinGame(static_cast<uint32_t>(selectedGameId_), outPlayerId);
    
    if (!success) {
        QMessageBox::critical(this, "Error", "Failed to join game.");
        loadGamesList();
        return;
    }
    
    playerId_ = outPlayerId;
    
    std::cout << "[JoinGameWindow] Joined game successfully. player_id=" << playerId_ << std::endl;
    
    this->hide();
    
    CarSelectionDialog carDialog(this);
    if (carDialog.exec() == QDialog::Accepted) {
        std::string selectedCar = carDialog.getSelectedCarType();
        lobbyClient_->selectCar(selectedCar);
    }
    
    GameLobbyWindow lobbyWindow(lobbyClient_, selectedGameName_, 
                                 static_cast<uint32_t>(selectedGameId_), 
                                 playerId_, false, this);
    int result = lobbyWindow.exec();
    
    if (result == QDialog::Accepted && lobbyWindow.wasGameStarted()) {
        gameStarted_ = true;
        
        auto connection = lobbyClient_->extractConnection();
        
        GameLauncher::launchWithConnection(std::move(connection));
        
        accept();
    } else {
        gameStarted_ = false;
        this->show();
        loadGamesList();
    }
}

void JoinGameWindow::onCancel() {
    reject();
}

void JoinGameWindow::onGameSelected(QListWidgetItem* item) {
    if (!item) {
        selectedGameId_ = -1;
        selectedGameName_ = "";
        updateJoinButtonState();
        return;
    }
    
    QVariant data = item->data(Qt::UserRole);
    QVariant nameData = item->data(Qt::UserRole + 1);
    
    if (data.isValid()) {
        selectedGameId_ = data.toInt();
        selectedGameName_ = nameData.toString();
    } else {
        selectedGameId_ = -1;
        selectedGameName_ = "";
    }
    updateJoinButtonState();
}

void JoinGameWindow::updateJoinButtonState() {
    ui->joinButton->setEnabled(selectedGameId_ >= 0);
}
