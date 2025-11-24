#ifndef CONNECTIONMENU_H
#define CONNECTIONMENU_H

#include <QDialog>
#include <string>

namespace Ui {
class ConnectionMenu;
}

class ConnectionMenu : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionMenu(QWidget *parent = nullptr);
    ~ConnectionMenu();
    
    std::string getHost() const;
    std::string getPort() const;

private slots:
    void onConnectClicked();

private:
    Ui::ConnectionMenu *ui;
    std::string host_;
    std::string port_;
};

#endif

