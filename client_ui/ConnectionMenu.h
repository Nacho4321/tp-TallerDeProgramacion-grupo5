#ifndef CONNECTIONMENU_H
#define CONNECTIONMENU_H

#include <QDialog>
#include <memory>
#include "ClientConnection.h"

namespace Ui {
class ConnectionMenu;
}

class ConnectionMenu : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionMenu(QWidget *parent = nullptr);
    ~ConnectionMenu();
    
    // Obtiene la conexión establecida
    std::shared_ptr<ClientConnection> connection() const;

private slots:
    void onConnectClicked();

private:
    Ui::ConnectionMenu *ui;
    std::shared_ptr<ClientConnection> connection_;
};

#endif

