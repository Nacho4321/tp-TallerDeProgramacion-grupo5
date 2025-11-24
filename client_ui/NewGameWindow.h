#ifndef NEWGAMEWINDOW_H
#define NEWGAMEWINDOW_H

#include <QDialog>
#include <memory>
#include "LobbyClient.h"

namespace Ui {
class NewGameWindow;
}

class NewGameWindow : public QDialog {
    Q_OBJECT

public:
    explicit NewGameWindow(std::shared_ptr<LobbyClient> lobby, QWidget* parent = nullptr);
    ~NewGameWindow();

private slots:
    void onCreate();
    void onBack();
    
private:
    Ui::NewGameWindow* ui;
    std::shared_ptr<LobbyClient> lobbyClient_;
};


#endif
