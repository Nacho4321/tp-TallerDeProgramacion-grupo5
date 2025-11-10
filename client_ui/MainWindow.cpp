#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ConnectionMenu.h"
#include "NewGameWindow.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , connection(nullptr)
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
        connection = dialog.connection();
    } else {
        QMessageBox::information(this, "Information", "You must connect to the server to continue");
        close();
    }
}

void MainWindow::onNewGameClicked() {
    if (!connection || !connection->isConnected()) {
        QMessageBox::warning(this, "Error", "No server connection");
        showConnectionDialog();
        return;
    }
    this->hide();
    NewGameWindow dlg(connection, this);
    const int r = dlg.exec();
    if (r != QDialog::Accepted) {
        this->show();
    } else {
        close();
    }
}

void MainWindow::onJoinGameClicked() {
    // TODO: Implementar mas adelante
    QMessageBox::information(this, "TODO", "Falta implementar");
}
