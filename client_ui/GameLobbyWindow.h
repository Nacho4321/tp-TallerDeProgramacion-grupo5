#ifndef GAMELOBBYWINDOW_H
#define GAMELOBBYWINDOW_H

#include <QDialog>
#include <QTimer>
#include <QCloseEvent>
#include <memory>
#include "LobbyClient.h"

namespace Ui {
class GameLobbyWindow;
}

class GameLobbyWindow : public QDialog {
    Q_OBJECT

public:
    explicit GameLobbyWindow(std::shared_ptr<LobbyClient> lobby, 
                             const QString& gameName,
                             uint32_t gameId,
                             uint32_t playerId,
                             bool isHost,
                             QWidget* parent = nullptr);
    ~GameLobbyWindow();
    
    bool wasGameStarted() const;
    bool wasForceClosed() const;
    
    uint32_t getGameId() const;
    uint32_t getPlayerId() const;

signals:
    void gameStartedSignal();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onStartGameClicked();
    void onPollTimer();

private:
    Ui::GameLobbyWindow* ui;
    std::shared_ptr<LobbyClient> lobbyClient_;
    QString gameName_;
    uint32_t gameId_;
    uint32_t playerId_;
    bool isHost_;
    bool gameStarted_;
    bool forceClosed_;
    
    QTimer* pollTimer_;
    
    void updateUI();
};

#endif
