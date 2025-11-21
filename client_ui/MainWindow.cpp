#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ConnectionMenu.h"
#include "NewGameWindow.h"
#include "JoinGameWindow.h"
#include "GameLauncher.h"

#include <QMessageBox>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , lobbyClient(std::make_shared<LobbyClient>())
{
    ui->setupUi(this);
    
    connect(ui->btnNewGame, &QPushButton::clicked, this, &MainWindow::onNewGameClicked);
    connect(ui->btnJoinGame, &QPushButton::clicked, this, &MainWindow::onJoinGameClicked);
    
    showConnectionDialog();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::showConnectionDialog() {
    ConnectionMenu dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        auto conn = dialog.connection();
        if (conn && conn->isConnected()) {
            // Usar la info de conexión para conectar el LobbyClient
            if (!lobbyClient->connect(conn->getAddress(), conn->getPort())) {
                QMessageBox::critical(this, "Error", "Failed to connect to server");
                close();
                return;
            }
        }
    } else {
        QMessageBox::information(this, "Information", "You must connect to the server to continue");
        close();
    }
}

void MainWindow::onNewGameClicked() {
    if (!lobbyClient || !lobbyClient->isConnected()) {
        QMessageBox::warning(this, "Error", "No server connection");
        showConnectionDialog();
        return;
    }
    this->hide();
    NewGameWindow dlg(lobbyClient, this);
    const int r = dlg.exec();
    if (r != QDialog::Accepted) {
        this->show();
    } else {
        close();
    }
}

void MainWindow::onJoinGameClicked() {
    if (!lobbyClient || !lobbyClient->isConnected()) {
        QMessageBox::warning(this, "Error", "No server connection");
        showConnectionDialog();
        return;
    }
    
    this->hide();
    JoinGameWindow dlg(lobbyClient, this);
    const int r = dlg.exec();
    
    if (r != QDialog::Accepted) {
        this->show();
    } else {
        // Usuario seleccionó una partida, lanzar el juego con JOIN
        int gameId = dlg.getSelectedGameId();
        std::string host = lobbyClient->getAddress();
        std::string port = lobbyClient->getPort();
        
        std::cout << "[MainWindow] Launching game with JOIN to game_id=" << gameId << std::endl;
        
        // Desconectar el lobby client antes de lanzar el juego
        lobbyClient->disconnect();
        
        GameLauncher::launchGameWithJoin(host, port, gameId);
        close();
    }
}
