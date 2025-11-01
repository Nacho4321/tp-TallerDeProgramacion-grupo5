#pragma once
#include <QMainWindow>

namespace Ui { 
    class MainWindow; 
}

class MainWindow : public QMainWindow {
    Q_OBJECT
    public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    private slots:
    void onNewGame();
    void onJoinGame();
    void onExit();
    void onConnectRequested(const QString& host, quint16 port);

    private:
    Ui::MainWindow* ui;
};
