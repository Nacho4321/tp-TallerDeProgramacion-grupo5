#include "JoinGameWindow.h"
#include "ui_JoinGameWindow.h"

#include <QMessageBox>
#include <iostream>

JoinGameWindow::JoinGameWindow(std::shared_ptr<LobbyClient> lobby, QWidget* parent)
    : QDialog(parent), 
      ui(new Ui::JoinGameWindow), 
      lobbyClient_(std::move(lobby)),
      selectedGameId_(-1) {
    
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
            ui->gamesListWidget->addItem(item);

        } else {
            for (const auto& game : games) {
                QString itemText = QString("🏎️  %1  |  ID: %2  |  👥 %3 player(s)")
                    .arg(QString::fromStdString(game.name))
                    .arg(game.game_id)
                    .arg(game.player_count);
                
                QListWidgetItem* item = new QListWidgetItem(itemText);
                item->setData(Qt::UserRole, static_cast<int>(game.game_id));
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
    
    accept(); 
}

void JoinGameWindow::onCancel() {
    reject();
}

void JoinGameWindow::onGameSelected(QListWidgetItem* item) {
    if (!item) {
        selectedGameId_ = -1;
        updateJoinButtonState();
        return;
    }
    
    QVariant data = item->data(Qt::UserRole);
    
    if (data.isValid()) {
        selectedGameId_ = data.toInt();
        std::cout << "[JoinGameWindow] Selected game ID: " << selectedGameId_ << std::endl;
    } else {
        selectedGameId_ = -1;
    }
    updateJoinButtonState();
}

void JoinGameWindow::updateJoinButtonState() {
    ui->joinButton->setEnabled(selectedGameId_ >= 0);
}
