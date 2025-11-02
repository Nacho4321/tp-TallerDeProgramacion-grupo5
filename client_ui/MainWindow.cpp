#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ConnectDialog.h"
#include <QMessageBox>
#include <QtCore/qresource.h>
#include <QApplication>

#include "../client/client.h"
#include <exception>
#include <iostream>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow), serverPort(0) { 
    ui->setupUi(this);
    Q_INIT_RESOURCE(resources); 

    connect(ui->btnNewGame, &QPushButton::clicked, this, &MainWindow::onNewGame);
    connect(ui->btnJoinGame, &QPushButton::clicked, this, &MainWindow::onJoinGame);
    connect(ui->btnExit, &QPushButton::clicked, this, &MainWindow::onExit);

    ConnectDialog dlg(this);
    connect(&dlg, &ConnectDialog::connectRequested, this, &MainWindow::onConnectRequested);
    dlg.exec();
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
    
    close();
    launchSDLClient();
}

void MainWindow::onJoinGame() {
    if (serverHost.isEmpty()) {
        QMessageBox::warning(this, "Error", "Connect to server first");
        return;
    }
    
    // Por ahora lo mismo q en New Game
    close();
    launchSDLClient();
}

void MainWindow::launchSDLClient() {
    try {
        std::string hostStr = serverHost.toStdString();
        std::string portStr = QString::number(serverPort).toStdString();
        
        // Crear y ejecutar el cliente SDL, se deberia realizar desde aca??
        Client client(hostStr.c_str(), portStr.c_str());
        client.start();
    } catch (const std::exception& e) {
        std::cerr << "Error in SDL client: " << e.what() << std::endl;
        QApplication::quit();
    }
}
