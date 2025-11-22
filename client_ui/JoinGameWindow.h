#ifndef JOINGAMEWINDOW_H
#define JOINGAMEWINDOW_H

#include <QDialog>
#include <QListWidgetItem>
#include <memory>
#include "LobbyClient.h"

namespace Ui {
class JoinGameWindow;
}

class JoinGameWindow : public QDialog {
    Q_OBJECT

public:
    explicit JoinGameWindow(std::shared_ptr<LobbyClient> lobby, QWidget* parent = nullptr);
    ~JoinGameWindow();

    int getSelectedGameId() const { return selectedGameId_; }

private slots:
    void onRefresh();
    void onJoin();
    void onCancel();
    void onGameSelected(QListWidgetItem* item);
    
private:
    Ui::JoinGameWindow* ui;
    std::shared_ptr<LobbyClient> lobbyClient_;
    int selectedGameId_;
    
    void loadGamesList();
    void updateJoinButtonState();
};

#endif
