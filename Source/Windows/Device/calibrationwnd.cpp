#include "calibrationwnd.h"
#include "ui_calibrationwnd.h"

CalibrationWnd::CalibrationWnd(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CalibrationWnd)
{
    ui->setupUi(this);

    calData = NULL;

    connect(ui->submitPusb, SIGNAL(clicked(bool)), this, SLOT(onSubmitPressed(bool)));
}

CalibrationWnd::~CalibrationWnd()
{
    delete ui;
}

void CalibrationWnd::showWnd()
{
    ui->adcVolRefLine->setText(QString::number(calData->adcVoltageRef));
    ui->currCorrLine->setText(QString::number(calData->currentCorrection));
    ui->currGainLine->setText(QString::number(calData->currentGain));
    ui->currShuntLine->setText(QString::number(calData->currentShunt));
    ui->volOffLine->setText(QString::number(calData->voltageOff));
    ui->volCorrLine->setText(QString::number(calData->voltageCorr));
    ui->volCOffLine->setText(QString::number(calData->voltageCurrOffset));
    show();
}

void CalibrationWnd::setCalibrationData(CalibrationData *aCalData)
{
    calData = aCalData;
}

void CalibrationWnd::onSubmitPressed(bool pressed)
{
    calData->adcVoltageRef     = ui->adcVolRefLine->text().toDouble();
    calData->currentCorrection = ui->currCorrLine->text().toDouble();
    calData->currentGain       = ui->currGainLine->text().toDouble();
    calData->currentShunt      = ui->currShuntLine->text().toDouble();
    calData->voltageOff        = ui->volOffLine->text().toDouble();
    calData->voltageCorr       = ui->volCorrLine->text().toDouble();
    calData->voltageCurrOffset = ui->volCOffLine->text().toDouble();

    emit sigCalibrationDataUpdated();
}
