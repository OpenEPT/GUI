#include "calibrationdata.h"

CalibrationData::CalibrationData(QObject *parent)
    : QObject{parent}
{
    dacOffset = 0;
    dacCorrection = 1;
}
CalibrationData &CalibrationData::operator=(const CalibrationData &other)
{
    if (this != &other) {
        adcVoltageRef = other.adcVoltageRef;
        voltageOff = other.voltageOff;
        voltageCorr = other.voltageCorr;
        voltageCurrOffset = other.voltageCurrOffset;
        currentCorrection = other.currentCorrection;
        currentGain = other.currentGain;
        currentShunt = other.currentShunt;
        dacOffset = other.dacOffset;
        dacCorrection = other.dacCorrection;
    }
    return *this;
}
CalibrationData::CalibrationData(const CalibrationData &other)
    : QObject(other.parent())
{
    adcVoltageRef = other.adcVoltageRef;
    voltageOff = other.voltageOff;
    voltageCorr = other.voltageCorr;
    voltageCurrOffset = other.voltageCurrOffset;
    currentCorrection = other.currentCorrection;
    currentGain = other.currentGain;
    currentShunt = other.currentShunt;
    dacOffset = other.dacOffset;
    dacCorrection = other.dacCorrection;
}
