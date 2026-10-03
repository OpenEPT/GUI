#include "calibrationwnd.h"
#include "ui_calibrationwnd.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

#define CALIBRATION_FIELD_WIDTH     90
#define CALIBRATION_UNIT_WIDTH      40
#define CALIBRATION_ROW_HEIGHT      26

CalibrationWnd::CalibrationWnd(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CalibrationWnd)
{
    ui->setupUi(this);

    calData = NULL;

    buildUi();

    connect(submitPusb, SIGNAL(clicked(bool)), this, SLOT(onSubmitPressed(bool)));
    connect(storePusb, SIGNAL(clicked(bool)), this, SLOT(onStorePressed(bool)));
    connect(autoPusb, &QPushButton::clicked, this, [this](){ emit sigStartAutoCalibration(); });
}

CalibrationWnd::~CalibrationWnd()
{
    delete ui;
}

QLineEdit* CalibrationWnd::createField(const QString &label, const QString &unit, QGridLayout *grid, int row)
{
    QLabel *nameLabel = new QLabel(label, this);
    QLineEdit *edit = new QLineEdit(this);
    QLabel *unitLabel = new QLabel(unit, this);

    edit->setFixedSize(CALIBRATION_FIELD_WIDTH, CALIBRATION_ROW_HEIGHT);
    unitLabel->setFixedWidth(CALIBRATION_UNIT_WIDTH);

    grid->addWidget(nameLabel, row, 0);
    grid->addWidget(edit, row, 1);
    grid->addWidget(unitLabel, row, 2);

    return edit;
}

void CalibrationWnd::buildUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *adcGroup = new QGroupBox("General ADC configuration", this);
    QGridLayout *adcGrid = new QGridLayout(adcGroup);
    adcVolRefLine = createField("ADC Voltage Ref", "V", adcGrid, 0);
    adcGrid->setColumnStretch(3, 1);
    adcGrid->setVerticalSpacing(8);

    QGroupBox *voltageGroup = new QGroupBox("Voltage channel calibration", this);
    QGridLayout *voltageGrid = new QGridLayout(voltageGroup);
    volOffLine  = createField("Voltage Offset", "V", voltageGrid, 0);
    volCorrLine = createField("Voltage Correction", "", voltageGrid, 1);
    voltageGrid->setColumnStretch(3, 1);
    voltageGrid->setVerticalSpacing(8);
    voltageGrid->setRowStretch(4, 1);

    QGroupBox *currentGroup = new QGroupBox("Current channel calibration", this);
    QGridLayout *currentGrid = new QGridLayout(currentGroup);
    volCOffLine   = createField("Voltage C. Offset", "V", currentGrid, 0);
    currCorrLine  = createField("Current Correction", "", currentGrid, 1);
    currGainLine  = createField("Current Gain", "", currentGrid, 2);
    currShuntLine = createField("Current Shunt", "ohm", currentGrid, 3);
    currentGrid->setColumnStretch(3, 1);
    currentGrid->setVerticalSpacing(8);
    currentGrid->setRowStretch(4, 1);

    QGroupBox *loadGroup = new QGroupBox("Load calibration", this);
    QGridLayout *loadGrid = new QGridLayout(loadGroup);
    dacOffLine = createField("Load DAC Offset", "mA", loadGrid, 0);
    dacCorLine = createField("Load DAC Correction", "", loadGrid, 1);
    loadGrid->setColumnStretch(3, 1);
    loadGrid->setVerticalSpacing(8);

    QHBoxLayout *channelsLayout = new QHBoxLayout();
    channelsLayout->addWidget(voltageGroup, 0, Qt::AlignTop);
    channelsLayout->addWidget(currentGroup, 0, Qt::AlignTop);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(loadGroup, 1);

    autoPusb = new QPushButton("Start auto calibration", this);
    autoPusb->setToolTip("Guided calibration of the voltage, current and load channels.\nThe acquisition has to be running");
    submitPusb = new QPushButton("Submit", this);
    storePusb = new QPushButton("Store", this);
    submitPusb->setToolTip("Apply the calibration parameters on the device");
    storePusb->setToolTip("Apply and permanently store the calibration parameters on the device");

    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(submitPusb);
    buttonLayout->addWidget(storePusb);

    bottomLayout->addLayout(buttonLayout);

    mainLayout->addWidget(adcGroup);
    mainLayout->addLayout(channelsLayout);
    mainLayout->addLayout(bottomLayout);
    mainLayout->addStretch();

    QHBoxLayout *autoLayout = new QHBoxLayout();
    autoLayout->addWidget(autoPusb);
    autoLayout->addStretch();
    mainLayout->addLayout(autoLayout);
}

void CalibrationWnd::showWnd()
{
    adcVolRefLine->setText(QString::number(calData->adcVoltageRef));
    currCorrLine->setText(QString::number(calData->currentCorrection));
    currGainLine->setText(QString::number(calData->currentGain));
    currShuntLine->setText(QString::number(calData->currentShunt));
    volOffLine->setText(QString::number(calData->voltageOff));
    volCorrLine->setText(QString::number(calData->voltageCorr));
    volCOffLine->setText(QString::number(calData->voltageCurrOffset));
    dacOffLine->setText(QString::number(calData->dacOffset));
    dacCorLine->setText(QString::number(calData->dacCorrection));
    show();
}

void CalibrationWnd::setCalibrationData(CalibrationData *aCalData)
{
    calData = aCalData;
}

void CalibrationWnd::onSubmitPressed(bool pressed)
{
    calData->adcVoltageRef     = adcVolRefLine->text().toDouble();
    calData->currentCorrection = currCorrLine->text().toDouble();
    calData->currentGain       = currGainLine->text().toDouble();
    calData->currentShunt      = currShuntLine->text().toDouble();
    calData->voltageOff        = volOffLine->text().toDouble();
    calData->voltageCorr       = volCorrLine->text().toDouble();
    calData->voltageCurrOffset = volCOffLine->text().toDouble();
    calData->dacOffset         = dacOffLine->text().toDouble();
    calData->dacCorrection     = dacCorLine->text().toDouble();

    emit sigCalibrationDataUpdated();
}

void CalibrationWnd::onStorePressed(bool pressed)
{
    onSubmitPressed(true);
    emit sigCalibrationStoreRequest();
}
