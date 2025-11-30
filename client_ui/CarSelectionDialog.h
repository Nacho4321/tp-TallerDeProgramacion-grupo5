#ifndef CAR_SELECTION_DIALOG_H
#define CAR_SELECTION_DIALOG_H

#include <QDialog>
#include <string>

namespace Ui {
class CarSelectionDialog;
}

class CarSelectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit CarSelectionDialog(QWidget* parent = nullptr);
    ~CarSelectionDialog();

    std::string getSelectedCarType() const;

private slots:
    void onPrevCar();
    void onNextCar();
    void onSelectCar();

private:
    void updateCarDisplay();

    Ui::CarSelectionDialog* ui;
    int currentIndex_;
};

#endif
