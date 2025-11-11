#include "ConnectionMenu.h"
#include "ui_ConnectionMenu.h"
#include <QMessageBox>

ConnectionMenu::ConnectionMenu(QWidget* parent)
    : QDialog(parent), ui(new Ui::ConnectionMenu),
      connection_(std::make_shared<ClientConnection>()) {
    ui->setupUi(this);
    ui->btnConnect->setDefault(true);
    connect(ui->btnConnect, &QPushButton::clicked, this, &ConnectionMenu::onConnectClicked);
    connect(ui->btnExit,    &QPushButton::clicked, this, &QDialog::reject);
}

ConnectionMenu::~ConnectionMenu() { 
    delete ui; 
}

void ConnectionMenu::onConnectClicked() {
    const QString host = ui->lineHost->text().trimmed();
    const QString port = ui->linePort->text().trimmed();
    
    if (host.isEmpty() || port.isEmpty()) {
        QMessageBox::warning(this, "Error", "Host and port must be provided.");
        return;
    }
    
    bool ok;
    int portNum = port.toInt(&ok);
    if (!ok || portNum < 1 || portNum > 65535) {
        QMessageBox::warning(this, "Error", "Port must be a number between 1 and 65535.");
        return;
    }

    connection_->setConnectionInfo(host.toStdString(), port.toStdString());
    accept();
}

std::shared_ptr<ClientConnection> ConnectionMenu::connection() const {
    return connection_;
}
