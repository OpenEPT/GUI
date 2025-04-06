#ifndef CALIBRATIONDATA_H
#define CALIBRATIONDATA_H

#include <QObject>

class CalibrationData : public QObject
{
    Q_OBJECT
public:
    explicit CalibrationData(QObject *parent = nullptr);

     CalibrationData(const CalibrationData &other);

     CalibrationData &operator=(const CalibrationData &other); // define if needed

signals:

public:
    double adcVoltageRef;
    double voltageOff;
    double voltageCorr;
    double voltageCurrOffset;
    double currentCorrection;
    double currentGain;
    double currentShunt;
};

#endif // CALIBRATIONDATA_H
