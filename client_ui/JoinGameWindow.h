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
    bool wasGameStarted() const { return gameStarted_; }
    
    uint32_t getPlayerId() const { return playerId_; }
    QString getSelectedGameName() const { return selectedGameName_; }

private slots:
    void onRefresh();
    void onJoin();
    void onCancel();
    void onGameSelected(QListWidgetItem* item);
    
private:
    Ui::JoinGameWindow* ui;
    std::shared_ptr<LobbyClient> lobbyClient_;
    int selectedGameId_;
    QString selectedGameName_;
    uint32_t playerId_;
    bool gameStarted_;
    
    void loadGamesList();
    void updateJoinButtonState();
};

#endif
