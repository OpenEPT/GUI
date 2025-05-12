#ifndef CHARGINGANALYSIS_H
#define CHARGINGANALYSIS_H

#include <QObject>
#include <QVector>
#include <QThread>

#define  CHARGINGANALYSIS_STATUS_CURRENT_IDLE_RANGE_MAX 1
#define  CHARGINGANALYSIS_STATUS_CURRENT_IDLE_RANGE_MIN -1

typedef enum
{
    CHARGINGANALYSIS_STATUS_UNKNOWN,
    CHARGINGANALYSIS_STATUS_IDLE,
    CHARGINGANALYSIS_STATUS_CHARGING,
    CHARGINGANALYSIS_STATUS_DISCHARGING
}charginganalysis_status_t;

class ChargingAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit ChargingAnalysis(QObject *parent = nullptr);

    void    clear();


signals:
    void    sigChargingStatusChanged(charginganalysis_status_t status);

public slots:
    void    onAddData(double current, double voltage);

private:
    charginganalysis_status_t currentChargingStatus;
    charginganalysis_status_t previousChargingStatus;

    QThread         *dataProcessingThread;

    QVector<double> currentValues;
    QVector<double> voltageValues;
    unsigned int    dataCounter;


    double          chargingCurrent;
    double          dischargingCurrent;
    QString         chargingRestTimeStr;
    QString         dischargingRestTimeStr;
    int             cyclesNo;

};

#endif // CHARGINGANALYSIS_H
