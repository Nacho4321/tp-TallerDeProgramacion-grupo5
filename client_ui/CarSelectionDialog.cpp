#include "CarSelectionDialog.h"
#include "ui_CarSelectionDialog.h"
#include "../common/constants.h"

#include <QPixmap>
#include <iostream>

CarSelectionDialog::CarSelectionDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::CarSelectionDialog)
    , currentIndex_(0)
{
    ui->setupUi(this);
    ui->carNameLabel->hide();
    
    connect(ui->prevButton, &QPushButton::clicked, this, &CarSelectionDialog::onPrevCar);
    connect(ui->nextButton, &QPushButton::clicked, this, &CarSelectionDialog::onNextCar);
    connect(ui->selectButton, &QPushButton::clicked, this, &CarSelectionDialog::onSelectCar);
    
    updateCarDisplay();
}

CarSelectionDialog::~CarSelectionDialog() {
    delete ui;
}

std::string CarSelectionDialog::getSelectedCarType() const {
    if (currentIndex_ >= 0 && currentIndex_ < CAR_TYPES_COUNT) {
        return CAR_TYPES[currentIndex_];
    }
    return GREEN_CAR;
}

void CarSelectionDialog::onPrevCar() {
    currentIndex_--;
    if (currentIndex_ < 0) {
        currentIndex_ = CAR_TYPES_COUNT - 1;
    }
    updateCarDisplay();
}

void CarSelectionDialog::onNextCar() {
    currentIndex_++;
    if (currentIndex_ >= CAR_TYPES_COUNT) {
        currentIndex_ = 0;
    }
    updateCarDisplay();
}

void CarSelectionDialog::onSelectCar() {
    accept();
}

void CarSelectionDialog::updateCarDisplay() {
    if (currentIndex_ < 0 || currentIndex_ >= CAR_TYPES_COUNT) {
        return;
    }
    
    QString imagePath = QString(":/img/car_%1.png").arg(currentIndex_ + 1);
    QPixmap pixmap(imagePath);
    
    if (!pixmap.isNull()) {
        pixmap = pixmap.scaled(ui->carImageLabel->size(), 
                               Qt::KeepAspectRatio, 
                               Qt::SmoothTransformation);
        ui->carImageLabel->setPixmap(pixmap);
    } else {
        ui->carImageLabel->setText(QString("[ %1 ]").arg(CAR_TYPES[currentIndex_]));
    }
    
    ui->counterLabel->setText(QString("%1 / %2")
        .arg(currentIndex_ + 1)
        .arg(CAR_TYPES_COUNT));
}
