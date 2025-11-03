#include "ConnectDialog.h"
#include "ui_ConnectDialog.h"
#include <QMessageBox>

ConnectDialog::ConnectDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::ConnectDialog) {
    ui->setupUi(this);

    connect(ui->btnConnect, &QPushButton::clicked, this, &ConnectDialog::handleConnect);
    connect(ui->btnExit,    &QPushButton::clicked, this, &QDialog::reject);
}

ConnectDialog::~ConnectDialog() { 
    delete ui; 
}

void ConnectDialog::handleConnect() {
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
