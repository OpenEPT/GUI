#ifndef CALIBRATIONWND_H
#define CALIBRATIONWND_H

#include <QWidget>
#include "Processing/calibrationdata.h"




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

private slots:
    void onSubmitPressed(bool pressed);

private:
    Ui::CalibrationWnd *ui;
    CalibrationData* calData;
};

#endif // CALIBRATIONWND_H
