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
    
    bool wasGameStarted() const;

    uint32_t getGameId() const;
    uint32_t getPlayerId() const;

private slots:
    void onCreate();
    void onBack();
    void onPrevMap();
    void onNextMap();
    
private:
    void updateMapDisplay();
    
    Ui::NewGameWindow* ui;
    std::shared_ptr<LobbyClient> lobbyClient_;
    bool gameStarted_;
    uint32_t gameId_;
    uint32_t playerId_;
    int currentMapIndex_;
};


#endif
