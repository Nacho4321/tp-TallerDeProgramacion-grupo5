#include "ConnectDialog.h"
#include "ui_ConnectDialog.h"

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
    quint16 port = ui->linePort->text().toUShort(&ok);
    if (!ok) port = 0;
    emit connectRequested(ui->lineHost->text(), port);
    accept();
}
