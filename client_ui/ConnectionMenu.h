#ifndef CONNECTIONMENU_H
#define CONNECTIONMENU_H

#include <QDialog>

namespace Ui { class ConnectionMenu; }

class ConnectionMenu : public QDialog {
    Q_OBJECT
public:
    explicit ConnectionMenu(QWidget* parent = nullptr);
    ~ConnectionMenu();

signals:
    void connectRequested(const QString& host, quint16 port);

private slots:
    void handleConnect();

private:
    Ui::ConnectionMenu* ui;
};

#endif
