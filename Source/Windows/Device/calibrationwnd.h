#ifndef CALIBRATIONWND_H
#define CALIBRATIONWND_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include "Processing/calibrationdata.h"

class QGridLayout;

namespace Ui {
class CalibrationWnd;
}

class CalibrationWnd : public QWidget
{
    Q_OBJECT

public:
    explicit CalibrationWnd(QWidget *parent = nullptr);
    ~CalibrationWnd();

    void setCalibrationData(CalibrationData* aCalData);

    void showWnd();

signals:
    void sigCalibrationDataUpdated();
    void sigCalibrationStoreRequest();
    void sigStartAutoCalibration();

private slots:
    void onSubmitPressed(bool pressed);
    void onStorePressed(bool pressed);

private:
    Ui::CalibrationWnd *ui;
    CalibrationData* calData;

    QLineEdit *adcVolRefLine;
    QLineEdit *volOffLine;
    QLineEdit *volCorrLine;
    QLineEdit *volCOffLine;
    QLineEdit *currCorrLine;
    QLineEdit *currGainLine;
    QLineEdit *currShuntLine;
    QLineEdit *dacOffLine;
    QLineEdit *dacCorLine;

    QPushButton *submitPusb;
    QPushButton *storePusb;
    QPushButton *autoPusb;

    QLineEdit* createField(const QString &label, const QString &unit, QGridLayout *grid, int row);
    void       buildUi();
};

#endif // CALIBRATIONWND_H
