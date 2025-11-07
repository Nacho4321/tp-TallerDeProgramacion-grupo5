#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ConnectionMenu.h"
#include "NewGameWindow.h" 

#include <QMessageBox>
#include <QtCore/qresource.h>
#include <QApplication>
#include <QTimer>

#include "../client/client.h"
#include <exception>
#include <iostream>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow), serverPort(0) { 
    ui->setupUi(this);
    Q_INIT_RESOURCE(resources); 
    
    connect(ui->btnNewGame, &QPushButton::clicked, this, &MainWindow::onNewGame);
    connect(ui->btnJoinGame, &QPushButton::clicked, this, &MainWindow::onJoinGame);
    connect(ui->btnExit, &QPushButton::clicked, this, &MainWindow::onExit);

    ConnectionMenu dlg(this);
    connect(&dlg, &ConnectionMenu::connectRequested, this, &MainWindow::onConnectRequested);
    
    dlg.exec();
    if (dlg.result() != QDialog::Accepted) {  // Si se toco exit en la conexion
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        return;
    }
}


MainWindow::~MainWindow() { 
    delete ui; 
}

void MainWindow::onExit() {
    close();
}

void MainWindow::onConnectRequested(const QString& host, quint16 port) {
    serverHost = host;
    serverPort = port;
    
    QMessageBox::information(this, "Connected",
        QString("Connected to %1:%2\n\n")  
            .arg(host).arg(port)
    );
}

void MainWindow::onNewGame() {
    if (serverHost.isEmpty()) {
        QMessageBox::warning(this, "Error", "Connect to server first");
        return;
    }

    this->hide(); 

    NewGameWindow dlg(this);
    
    // Para volver a mostrar eld Main Window
    QObject::connect(&dlg, &NewGameWindow::backRequested,
                     this, [this]{ this->show(); });

    QObject::connect(&dlg, &NewGameWindow::createGameRequested,
                     this, [this](const QString& room, int players, const QString& map){
        QMessageBox::information(this, "New Game",
            QString("Room: %1\nPlayers: %2\nMap: %3").arg(room).arg(players).arg(map));
    });

    const int res = dlg.exec();         
    if (res != QDialog::Accepted) {
        this->show();
    }
}

void MainWindow::onJoinGame() {
    // TODO
    if (serverHost.isEmpty()) {
        QMessageBox::warning(this, "Error", "Connect to server first");
        return;
    }
    close();
}
