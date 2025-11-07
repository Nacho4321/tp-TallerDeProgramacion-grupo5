#include "NewGameWindow.h"
#include "ui_NewGameWindow.h"

#include <QMessageBox>

NewGameWindow::NewGameWindow(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::NewGameWindow) {
    ui->setupUi(this);

    // Defaults tempoarles
    ui->lineRoomName->setText("lobby-1");
    ui->cmbPlayers->addItems({"2", "3", "4"});
    ui->cmbPlayers->setCurrentIndex(0);
    ui->cmbMap->addItems({"map1", "map2", "map3"});
    ui->cmbMap->setCurrentText("map1");

    connect(ui->btnCreate, &QPushButton::clicked, this, &NewGameWindow::onCreate);
    connect(ui->btnBack,   &QPushButton::clicked, this, &NewGameWindow::onBack);

    ui->btnCreate->setDefault(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

NewGameWindow::~NewGameWindow() {
    delete ui;
}

void NewGameWindow::onCreate() {
    const QString room = ui->lineRoomName->text().trimmed();
    const int players  = playersFromCombo();
    const QString map  = ui->cmbMap->currentText().trimmed();

    if (room.isEmpty()) {
        QMessageBox::warning(this, "New Game", "Room name required.");
        return;
    }
    if (players < 2 || players > 4) {
        QMessageBox::warning(this, "New Game", "Players must be 2–4.");
        return;
    }
    if (map.isEmpty()) {
        QMessageBox::warning(this, "New Game", "Select a map.");
        return;
    }

    emit createGameRequested(room, players, map);
    accept();
}

void NewGameWindow::onBack() {
    emit backRequested();
    reject();
}

int NewGameWindow::playersFromCombo() const {
    bool ok = false;
    const int n = ui->cmbPlayers->currentText().toInt(&ok);
    return ok ? n : 2;
}
