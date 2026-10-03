#ifndef AUTOCALIBRATIONWND_H
#define AUTOCALIBRATIONWND_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QTimer>
#include <QPlainTextEdit>

#include "Processing/calibrationdata.h"

typedef enum
{
    AUTOCAL_STEP_IDLE = 0,
    AUTOCAL_STEP_DISCONNECT_INPUT,
    AUTOCAL_STEP_SET_REFERENCE,
    AUTOCAL_STEP_VOLTAGE_TUNE,
    AUTOCAL_STEP_CURRENT_OFFSET,
    AUTOCAL_STEP_JUMPER_BATTERY,
    AUTOCAL_STEP_CONNECT_SOURCE,
    AUTOCAL_STEP_CURRENT_SPAN,
    AUTOCAL_STEP_CURRENT_ZERO2,
    AUTOCAL_STEP_LOAD_TUNE,
    AUTOCAL_STEP_DONE
}autocal_step_t;

class AutoCalibrationWnd : public QWidget
{
    Q_OBJECT

public:
    explicit            AutoCalibrationWnd(QWidget *parent = nullptr);

    void                setCalibrationData(CalibrationData *aCalData);
    void                setAcquisitionActive(bool active);
    void                setLoadDisabled(bool disabled);
    void                startCalibration();

signals:
    void                sigApplyCalibration();
    void                sigSetLoadCurrent(int mA);
    void                sigSetLoadEnabled(bool enabled);
    void                sigResetProtection();
    void                sigCalibrationFinished(bool success);

public slots:
    void                onNewStatistics(double voltageAvg, double currentAvg);
    void                onLoadEnableResult(bool ok);
    void                appendLog(QString message);

private slots:
    void                onNextClicked();
    void                onBackClicked();
    void                onCancelClicked();
    void                onReadyClicked();
    void                onLoadStepTick();

private:
    QLabel             *titleLabel;
    QLabel             *stepLabel;
    QLabel             *instructionLabel;
    QLabel             *imageLabel;
    QLabel             *liveLabel;
    QLabel             *resultLabel;
    QPlainTextEdit     *logView;
    QPushButton        *nextButton;
    QPushButton        *backButton;
    QPushButton        *cancelButton;
    QPushButton        *readyButton;

    CalibrationData    *calData;
    autocal_step_t      step;
    bool                acquisitionActive;
    bool                loadDisabled;

    CalibrationData     startSnapshot;
    bool                snapshotValid;

    double              referenceVoltage;
    bool                externalReference;
    double              voltageAvg;
    double              currentAvg;
    bool                statsValid;

    QVector<double>     voltageHistory;
    QVector<double>     currentHistory;

    QTimer             *loadTimer;
    int                 loadPhase;
    QVector<double>     loadRequested;
    QVector<double>     loadMeasured;
    int                 loadSettleTicks;
    double              spanSumMT;
    double              spanSumMM;
    int                 currentSpanPoints;
    int                 loadPoints;

    void                goToStep(autocal_step_t newStep);
    void                updateView();
    void                pushHistory(double v, double c);
    bool                voltageStable(double *mean, double spreadLimit);
    bool                currentStable(double *mean, double spreadLimit);
    double              maxLoadCurrentMa();
    int                 loadPointCount();
    void                failAndRestart(QString reason);
    autocal_step_t      modeStartStep(autocal_step_t s);
    void                setImage(QString fileName);
    QString             buildReport();
};

#endif // AUTOCALIBRATIONWND_H
