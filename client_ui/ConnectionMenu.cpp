#include "ConnectionMenu.h"
#include "ui_ConnectionMenu.h"
#include <QMessageBox>

ConnectionMenu::ConnectionMenu(QWidget* parent)
    : QDialog(parent), ui(new Ui::ConnectionMenu), host_(), port_() {
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

    host_ = host.toStdString();
    port_ = port.toStdString();
    accept();
}


std::string ConnectionMenu::getHost() const {
    return host_;
}


std::string ConnectionMenu::getPort() const {
    return port_;
}
