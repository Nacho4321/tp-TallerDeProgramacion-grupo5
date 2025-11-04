#include "ConnectionMenu.h"
#include "ui_ConnectionMenu.h"
#include <QMessageBox>

ConnectionMenu::ConnectionMenu(QWidget* parent)
    : QDialog(parent), ui(new Ui::ConnectionMenu) {
    ui->setupUi(this);

    ui->btnConnect->setDefault(true);
    ui->btnConnect->setAutoDefault(true);
    connect(ui->btnConnect, &QPushButton::clicked, this, &ConnectionMenu::handleConnect);
    connect(ui->btnExit,    &QPushButton::clicked, this, &QDialog::reject);
}

ConnectionMenu::~ConnectionMenu() {
    delete ui;
}

void ConnectionMenu::handleConnect() {
    bool ok = false;
    const QString host = ui->lineHost->text().trimmed();
    const quint16 port = ui->linePort->text().toUShort(&ok);
    if (!ok || port == 0 || host.isEmpty()) {
        QMessageBox::warning(this, "Connection", "Please enter a valid host and port.");
        return;
    }
    emit connectRequested(ui->lineHost->text(), port);
    accept();
}
