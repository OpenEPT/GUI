#include "charginganalysis.h"

ChargingAnalysis::ChargingAnalysis(QObject *parent)
    : QObject{parent}
{
    dataProcessingThread = new QThread(this);
    this->moveToThread(dataProcessingThread);
    dataProcessingThread->setObjectName("OpenEPT - Charging data processing thread");
    dataProcessingThread->start();

    clear();
}

void ChargingAnalysis::clear()
{
    currentValues.clear();
    voltageValues.clear();
    dataCounter = 0;
    currentChargingStatus = CHARGINGANALYSIS_STATUS_UNKNOWN;
    previousChargingStatus = CHARGINGANALYSIS_STATUS_UNKNOWN;

}

void ChargingAnalysis::onAddData(double current, double voltage)
{
    currentValues.append(current);
    voltageValues.append(voltage);
    dataCounter += 1;

    if((current > CHARGINGANALYSIS_STATUS_CURRENT_IDLE_RANGE_MIN) &&
            (current < CHARGINGANALYSIS_STATUS_CURRENT_IDLE_RANGE_MAX))
    {
        currentChargingStatus = CHARGINGANALYSIS_STATUS_IDLE;
    }
    if(current > CHARGINGANALYSIS_STATUS_CURRENT_IDLE_RANGE_MAX)
    {
        currentChargingStatus = CHARGINGANALYSIS_STATUS_DISCHARGING;
    }
    if(current < CHARGINGANALYSIS_STATUS_CURRENT_IDLE_RANGE_MIN)
    {
        currentChargingStatus = CHARGINGANALYSIS_STATUS_CHARGING;
    }

    if(previousChargingStatus != currentChargingStatus)
    {
        emit sigChargingStatusChanged(currentChargingStatus);
        previousChargingStatus = currentChargingStatus;
    }
}
