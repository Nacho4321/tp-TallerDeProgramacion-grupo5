#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "LobbyClient.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNewGameClicked();
    void onJoinGameClicked();
    void onExitClicked();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<LobbyClient> lobbyClient;
    
    void showConnectionDialog();
};

#endif
