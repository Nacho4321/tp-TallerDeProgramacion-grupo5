#ifndef NEWGAMEWINDOW_H
#define NEWGAMEWINDOW_H

#include <QDialog>

namespace Ui { class NewGameWindow; }

class NewGameWindow : public QDialog {
    Q_OBJECT
public:
    explicit NewGameWindow(QWidget* parent = nullptr);
    ~NewGameWindow();

signals:
    void createGameRequested(const QString& roomName, int maxPlayers, const QString& mapId);
    void backRequested();

private slots:
    void onCreate();
    void onBack();

private:
    Ui::NewGameWindow* ui;
    int playersFromCombo() const;
};

#endif
