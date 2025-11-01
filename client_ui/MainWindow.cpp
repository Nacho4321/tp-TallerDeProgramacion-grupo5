#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ConnectDialog.h"
#include <QMessageBox>
#include <QtCore/qresource.h>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) { 
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


void MainWindow::onNewGame() {
    QMessageBox::information(this, "New Game", "TODO: crear partida");
}


void MainWindow::onJoinGame() {
    QMessageBox::information(this, "Join Game", "TODO: unirse a partida");
}


void MainWindow::onExit() {
    close();
}


void MainWindow::onConnectRequested(const QString& host, quint16 port) {
    QMessageBox::information(this, "Connect",
        QString("Conectar a %1:%2 — luego: lanzar threads y handshake.")
            .arg(host).arg(port)
    );
}
