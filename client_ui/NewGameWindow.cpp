#include "NewGameWindow.h"
#include "ui_NewGameWindow.h"
#include "GameLauncher.h"

#include <QMessageBox>
#include <iostream>

NewGameWindow::NewGameWindow(std::shared_ptr<LobbyClient> lobby, QWidget* parent)
    : QDialog(parent), ui(new Ui::NewGameWindow), lobbyClient_(std::move(lobby)) {
    ui->setupUi(this);
    ui->gameNameLineEdit->setText("Lobby");
    ui->maxPlayersSpinBox->setMinimum(1);
    ui->maxPlayersSpinBox->setMaximum(8);
    ui->maxPlayersSpinBox->setValue(2);

    connect(ui->createButton, &QPushButton::clicked, this, &NewGameWindow::onCreate);
    connect(ui->cancelButton, &QPushButton::clicked, this, &NewGameWindow::onBack);
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

    if (gameName.isEmpty()) {
        QMessageBox::warning(this, "New Game", "Game name is missing.");
        return;
    }

    std::string host = lobbyClient_->getAddress();
    std::string port = lobbyClient_->getPort();
    
    if (host.empty() || port.empty()) {
        QMessageBox::warning(this, "Error", "No server information configured.");
        return;
    }
    
    accept();

    std::cout << "Creating new game: " << gameName.toStdString()
              << " with max players: " << maxPlayers << std::endl;
    
    // Desconectar lobby antes de lanzar el juego
    lobbyClient_->disconnect();
              
    // Lanzar el cliente SDL con el nombre del juego
    GameLauncher::launchGame(host, port, gameName.toStdString());
}

void NewGameWindow::onBack() { 
    reject(); 
}
