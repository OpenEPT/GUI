#include "devicewnd.h"
#include "ui_devicewnd.h"
#include "Windows/Console/consolewnd.h"
#include <QFileDialog>
#include <QNetworkInterface>
#include <QOpenGLWidget>

/*TODO: Declare this in config file*/
#define PLOT_MINIMUM_SIZE_WIDTH     200
#define PLOT_MINIMUM_SIZE_HEIGHT    100


DeviceWnd::DeviceWnd(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceWnd)
{

    QFont appFont = this->font();  // Get the default font
    appFont.setPointSize(10);    // Set font size to 14 (or any desired size)
    this->setFont(appFont);
    ui->setupUi(this);
    adcOptions = new QStringList();
    *adcOptions
        <<""
        <<"Int"
        <<"Ext"
        ;
    ui->adcComb->addItems(*adcOptions);

    resolutionOptions = new QStringList();
    *resolutionOptions
        <<""
        <<"16"
        <<"14"
        <<"12"
        <<"10"
        ;
    ui->resolutionComb->addItems(*resolutionOptions);

    /* Set default Value for ADC Clock Div Comb*/
    clockDivOptions = new QStringList();
    *clockDivOptions
        <<""
        <<"1"
        <<"2"
        <<"4"
        <<"8"
        <<"16"
        <<"32"
        <<"64"
        <<"128"
        <<"256"
        ;
    ui->clockDivComb->addItems(*clockDivOptions);

    /* Set default Value for ADC Sample Time Comb*/
    sampleTimeOptions = new QStringList();
    *sampleTimeOptions
        <<""
        <<"1C5"
        <<"2C5"
        <<"8C5"
        <<"16C5"
        <<"32C5"
        <<"64C5"
        <<"387C5"
        <<"810C5"
        ;
    ui->sampleTimeComb->addItems(*sampleTimeOptions);

    /* Set default Value for ADC Averaging Options Comb*/
    averaginOptions = new QStringList();
    *averaginOptions
        <<""
        <<"1"
        <<"2"
        <<"4"
        <<"8"
        <<"16"
        <<"32"
        <<"64"
        <<"128"
        <<"256"
        <<"512"
        <<"1024"
        ;

    networkInterfacesNames = new QStringList();
    *networkInterfacesNames << "";

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach(QNetworkInterface a, interfaces)
    {
        QList<QNetworkAddressEntry> allEntries = a.addressEntries();
        if(a.flags() & QNetworkInterface::IsUp){
            qDebug() << "Interface Name:" << a.name();
            QNetworkAddressEntry entry;
            foreach (entry, allEntries) {
                if( entry.ip().protocol() == QAbstractSocket::IPv4Protocol){
                    *networkInterfacesNames <<   "<" + a.name() + ">:" + entry.ip().toString();
                }
            }
        }
    }

    ui->streamServerInterfComb->addItems(*networkInterfacesNames);

    advanceConfigurationWnd  = new AdvanceConfigurationWnd();
    advanceConfigurationWnd->hide();
    //Prethodno se lista kreira dinamicki
    advanceConfigurationWnd->assignAdcOptionsList(adcOptions);
    advanceConfigurationWnd->assignResolutionOptionsList(resolutionOptions);
    advanceConfigurationWnd->assignClockDivOptionsList(clockDivOptions);
    advanceConfigurationWnd->assignSampleTimeOptionsList(sampleTimeOptions);
    advanceConfigurationWnd->assignAvgRatioOptionsList(averaginOptions);

    dataAnalyzer            = new DataStatistics();
    energyControlWnd        = new EnergyControlWnd();

    connect(energyControlWnd, SIGNAL(sigLoadStatusChanged(bool)), this, SLOT(onLoadStatusChanged(bool)));
    connect(energyControlWnd, SIGNAL(sigPPathStatusChanged(bool)), this, SLOT(onPPathStatusChanged(bool)));
    connect(energyControlWnd, SIGNAL(sigBatteryStatusChanged(bool)), this, SLOT(onBatteryStatusChanged(bool)));
    connect(energyControlWnd, SIGNAL(sigResetProtection()), this, SLOT(onResetProtection()));
    connect(energyControlWnd, SIGNAL(sigLoadCurrentStatusChanged(bool)), this, SLOT(onLoadCurrentStatusChanged(bool)));
    connect(energyControlWnd, SIGNAL(sigLoadCurrentChanged(unsigned int)), this, SLOT(onLoadCurrentChanged(unsigned int)));
    connect(energyControlWnd, SIGNAL(sigChargingCurrentStatusChanged(bool)), this, SLOT(onChargingCurrentStatusChanged(bool)));
    connect(energyControlWnd, SIGNAL(sigChargingCurrentChanged(unsigned int)), this, SLOT(onChargingCurrentChanged(unsigned int)));
    connect(energyControlWnd, SIGNAL(sigChargingTermCurrentChanged(unsigned int)), this, SLOT(onChargingTermCurrentChanged(unsigned int)));
    connect(energyControlWnd, SIGNAL(sigChargingTermVoltageChanged(float)), this, SLOT(onChargingTermVoltageChanged(float)));
    connect(energyControlWnd, SIGNAL(sigChDschWriteToFileToogled(bool)), this, SLOT(onChDschSaveToFileChanged(bool)));



    connect(ui->advanceOptionPusb, SIGNAL(clicked(bool)), this, SLOT(onAdvanceConfigurationButtonPressed(bool)));

    voltageChart             = new Plot(PLOT_MINIMUM_SIZE_WIDTH/2, PLOT_MINIMUM_SIZE_HEIGHT);
    voltageChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    voltageChart->setTitle("Voltage");
    voltageChart->setYLabel("[V]");
    voltageChart->setXLabel("[ms]");
    voltageChart->scatterAddGraph();
    /* Make plot for consumption presentation */
    currentChart         = new Plot(PLOT_MINIMUM_SIZE_WIDTH/2, PLOT_MINIMUM_SIZE_HEIGHT);
    currentChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    currentChart->setTitle("Current");
    currentChart->setYLabel("[mA]");
    currentChart->setXLabel("[ms]");

    currentChart->scatterAddGraph();


    consumptionChart         = new Plot(PLOT_MINIMUM_SIZE_WIDTH, PLOT_MINIMUM_SIZE_HEIGHT);
    consumptionChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    consumptionChart->setTitle("Consumption");
    consumptionChart->setYLabel("[mAh]");
    consumptionChart->setXLabel("[ms]");

    consumptionChart->scatterAddGraph();

    ui->maxNumOfPacketsLine->setText(QString::number(DEVICEWND_DEFAULT_MAX_NUMBER_OF_BUFFERS));
    ui->statisticsPacketCounterLabe2->setText(QString::number(0));
    ui->statisticsDropRateProb->setValue(0);
    ui->statisticsSamplingPeriodLabe2->setText(QString::number(0));
    ui->samplingPeriodLine->setText(QString::number(0));

    /*Group consumption type selecrion radio buttons*/
    consumptionTypeSelection = new QButtonGroup();
    consumptionTypeSelection->addButton(ui->currentRadb);
    consumptionTypeSelection->addButton(ui->cumulativeRadb);
    consumptionTypeSelection->setId(ui->currentRadb, 1);
    consumptionTypeSelection->setId(ui->cumulativeRadb, 2);
    measurementTypeSelection = new QButtonGroup();
    measurementTypeSelection->addButton(ui->measurementTypeCurrentRadb);
    measurementTypeSelection->addButton(ui->measurementTypeVoltageRadb);
    measurementTypeSelection->setId(ui->measurementTypeVoltageRadb, 1);
    measurementTypeSelection->setId(ui->measurementTypeCurrentRadb, 2);

    setStatisticsElapsedTime(0);

    ui->currentRadb->setChecked(true);
    setConsumptionType(DEVICE_CONSUMPTION_TYPE_CURRENT);
    ui->measurementTypeVoltageRadb->setChecked(true);
    setMeasurementType(DEVICE_MEASUREMENT_TYPE_VOLTAGE);

    setDeviceState(DEVICE_STATE_UNDEFINED);

    consoleWnd  = new ConsoleWnd();

    calibrationWnd = new CalibrationWnd();

    ui->GraphicsTopHorl->addWidget(voltageChart);
    ui->GraphicsTopHorl->addWidget(currentChart);
    ui->GraphicsBottomVerl->addWidget(consumptionChart, Qt::AlignCenter);
    setDeviceInterfaceSelectionState(DEVICE_INTERFACE_SELECTION_STATE_UNDEFINED);

    connect(ui->saveToFileCheb, SIGNAL(stateChanged(int)), this, SLOT(onSaveToFileChanged(int)));
    connect(ui->EPControlEnableCheb, SIGNAL(stateChanged(int)), this, SLOT(onEPEnableChanged(int)));
    connect(ui->consNamePusb, SIGNAL(clicked(bool)), this, SLOT(onSetConsumptionName()));
    connect(ui->startPusb, SIGNAL(clicked(bool)), this, SLOT(onStartAcquisition()));
    connect(ui->pausePusb, SIGNAL(clicked(bool)), this, SLOT(onPauseAcquisition()));
    connect(ui->stopPusb, SIGNAL(clicked(bool)), this, SLOT(onStopAcquisiton()));
    connect(ui->refreshPusb, SIGNAL(clicked(bool)), this, SLOT(onRefreshAcquisiton()));
    connect(ui->ConsolePusb, SIGNAL(clicked(bool)), this, SLOT(onConsolePressed()));
    connect(ui->DataStatisticsPushb, SIGNAL(clicked(bool)), this, SLOT(onDataAnalyzerPressed()));

    connect(ui->clockDivComb,           SIGNAL(currentTextChanged(QString)),        this, SLOT(onClockDivChanged(QString)));
    connect(ui->sampleTimeComb,         SIGNAL(currentTextChanged(QString)),        this, SLOT(onSampleTimeChanged(QString)));
    connect(ui->resolutionComb,         SIGNAL(currentTextChanged(QString)),        this, SLOT(onResolutionChanged(QString)));
    connect(ui->samplingPeriodLine,     SIGNAL(returnPressed()),                    this, SLOT(onSamplingPeriodChanged()));
    connect(ui->adcComb,                SIGNAL(currentTextChanged(QString)),        this, SLOT(onADCChanged(QString)));
    connect(ui->streamServerInterfComb, SIGNAL(currentTextChanged(QString)),        this, SLOT(onInterfaceChanged(QString)));
    connect(ui->maxNumOfPacketsLine,    SIGNAL(editingFinished()),                  this, SLOT(onMaxNumberOfBuffersChanged()));
    connect(consumptionTypeSelection,   SIGNAL(buttonClicked(QAbstractButton*)),    this, SLOT(onConsumptionTypeChanged(QAbstractButton*)));
    connect(measurementTypeSelection,   SIGNAL(buttonClicked(QAbstractButton*)),    this, SLOT(onMeasurementTypeChanged(QAbstractButton*)));


    connect(advanceConfigurationWnd, SIGNAL(sigAdvConfigurationChanged(QVariant)), this, SLOT(onAdvConfigurationChanged(QVariant)));
    connect(advanceConfigurationWnd, SIGNAL(sigAdvConfigurationRequested()), this, SLOT(onAdvConfigurationReqested()));

    connect(consoleWnd, SIGNAL(sigControlMsgSend(QString)), this, SLOT(onNewControlMsgRcvd(QString)));

    connect(calibrationWnd, SIGNAL(sigCalibrationDataUpdated()), this, SLOT(onCalibrationUpdated()));

    connect(ui->dischargeControlPusb1, SIGNAL(clicked(bool)), this, SLOT(onCalibrationButtonPressed(bool)));
    connect(ui->dischargeControlPusb2, SIGNAL(clicked(bool)), this, SLOT(onEnenergyControlButtonPressed(bool)));
}

void    DeviceWnd::onNewControlMsgRcvd(QString text)
{
    /* emit signal to deviceContrainer -> */
    emit sigNewControlMessageRcvd(text);
}

void DeviceWnd::onPlotScatterNameAndKey(QString name, double key)
{
    emit sigScatterNameAndKey(name, key);
}

void DeviceWnd::onCalibrationUpdated()
{
    emit sigCalibrationUpdated();
}

void DeviceWnd::onSetConsumptionName()
{
    emit sigConsumptionProfileNameChanged(ui->consNameLine->text());
}

void DeviceWnd::onInterfaceChanged(QString interfaceInfo)
{
    QString ip;
    QStringList interfaceInfoParts;
    interfaceInfoParts = interfaceInfo.split(":");
    ip = interfaceInfoParts[1];
    emit sigNewInterfaceSelected(ip);
}

void DeviceWnd::onAdvConfigurationChanged(QVariant aConfig)
{
    emit sigAdvConfigurationChanged(aConfig);
}

void DeviceWnd::onAdvConfigurationReqested(void)
{
    emit sigAdvConfigurationReqested();
}

void DeviceWnd::onMaxNumberOfBuffersChanged()
{
    QString maxNumberOfSamplesBuffers = ui->maxNumOfPacketsLine->text();
    emit sigMaxNumberOfBuffersChanged(maxNumberOfSamplesBuffers.toInt());

}

void DeviceWnd::onEPEnableChanged(int value)
{
    emit sigEPEnable(ui->EPControlEnableCheb->isChecked());
}

void DeviceWnd::onLoadCurrentStatusChanged(bool newState)
{
    emit sigLoadCurrentStatusChanged(newState);
}

void DeviceWnd::onLoadCurrentChanged(unsigned int current)
{
    emit sigLoadCurrentChanged(current);
}

void DeviceWnd::onChargingCurrentStatusChanged(bool newState)
{
    emit sigChargingCurrentStatusChanged(newState);
}

void DeviceWnd::onChargingCurrentChanged(unsigned int current)
{
    emit sigChargingCurrentChanged(current);
}

void DeviceWnd::onChargingTermCurrentChanged(unsigned int current)
{
    emit sigChargingTermCurrentChanged(current);
}

void DeviceWnd::onChargingTermVoltageChanged(float voltage)
{
    emit sigChargingTermVoltageChanged(voltage);
}

void DeviceWnd::onLoadStatusChanged(bool status)
{
    emit sigLoadStatusChanged(status);
}

void DeviceWnd::onPPathStatusChanged(bool status)
{
    emit sigPPathStatusChanged( status);
}

void DeviceWnd::onBatteryStatusChanged(bool status)
{

    emit sigBatteryStatusChanged(status);
}

void DeviceWnd::onResetProtection()
{
    emit sigResetProtection();
}

void DeviceWnd::onConsumptionProfileNameChanged()
{
}

void DeviceWnd::onConsumptionTypeChanged(QAbstractButton* button)
{
    int id = consumptionTypeSelection->id(button);
    switch(id)
    {
    case 1:
        emit sigConsumptionTypeChanged("Current");
        setConsumptionType(DEVICE_CONSUMPTION_TYPE_CURRENT);
        break;
    case 2:
        emit sigConsumptionTypeChanged("Cumulative");
        setConsumptionType(DEVICE_CONSUMPTION_TYPE_CUMULATIVE);
        break;
    default:
        emit sigConsumptionTypeChanged("Undef");
        setConsumptionType(DEVICE_CONSUMPTION_TYPE_UNDEF);
        break;
    }
}

void DeviceWnd::onMeasurementTypeChanged(QAbstractButton *button)
{
    int id = measurementTypeSelection->id(button);
    switch(id)
    {
    case 1:
        emit sigMeasurementTypeChanged("Voltage");
        setMeasurementType(DEVICE_MEASUREMENT_TYPE_VOLTAGE);
        break;
    case 2:
        emit sigMeasurementTypeChanged("Current");
        setMeasurementType(DEVICE_MEASUREMENT_TYPE_CURRENT);
        break;
    default:
        emit sigMeasurementTypeChanged("Undef");
        setMeasurementType(DEVICE_MEASUREMENT_TYPE_UNDEF);
        break;
    }
}

void DeviceWnd::onResolutionChanged(QString resolution)
{
    advanceConfigurationWnd->setResolution(resolution);
    emit sigResolutionChanged(ui->resolutionComb->currentText());
}

void DeviceWnd::onADCChanged(QString adc)
{
    emit sigADCChanged(ui->adcComb->currentText());
}

void DeviceWnd::onClockDivChanged(QString aClockDiv)
{
    advanceConfigurationWnd->setClockDiv(aClockDiv);
    emit sigClockDivChanged(ui->clockDivComb->currentText());
}

void DeviceWnd::onSampleTimeChanged(QString aSTime)
{
    advanceConfigurationWnd->setChSampleTime(aSTime);
    emit sigSampleTimeChanged(ui->sampleTimeComb->currentText());
}

void DeviceWnd::onSamplingPeriodChanged()
{
    QString time = ui->samplingPeriodLine->text();
    advanceConfigurationWnd->setSamplingTime(time);
    ui->statisticsSamplingPeriodLabe2->setText(time);
    emit sigSamplingPeriodChanged(time);
}


void    DeviceWnd::onAdvanceConfigurationButtonPressed(bool pressed)
{
    advanceConfigurationWnd->show();
}

void DeviceWnd::onCalibrationButtonPressed(bool pressed)
{
    calibrationWnd->showWnd();
}

void DeviceWnd::onEnenergyControlButtonPressed(bool pressed)
{
    energyControlWnd->show();
}

void    DeviceWnd::onSaveToFileChanged(int value)
{
    if(value == Qt::Checked)
    {
        ui->consNamePusb->setEnabled(true);
        ui->consNameLab->setEnabled(true);
        ui->consNameLine->setEnabled(true);
        ui->consNameLine->setPlaceholderText("Set Consumption Name");
        emit sigSaveToFileEnabled(true);
    }else
    {
        ui->consNamePusb->setEnabled(false);
        ui->consNameLab->setEnabled(false);
        ui->consNameLine->setEnabled(false);
        ui->consNameLine->setPlaceholderText("Enable \"Save to file\"");
        emit sigSaveToFileEnabled(false);
    }
}

void DeviceWnd::onChDschSaveToFileChanged(bool state)
{
    emit sigChDschSaveToFileToggled(state);
}

void DeviceWnd::onConsolePressed()
{
    consoleWnd->show();
}

void DeviceWnd::onDataAnalyzerPressed()
{
    dataAnalyzer->show();
}

void DeviceWnd::onStartAcquisition()
{
    emit sigStartAcquisition();
}

void DeviceWnd::onPauseAcquisition()
{
    emit sigPauseAcquisition();
}

void DeviceWnd::onStopAcquisiton()
{
    voltageChart->clear();
    currentChart->clear();
    consumptionChart->clear();
    emit sigStopAcquisition();
}

void DeviceWnd::onRefreshAcquisiton()
{
    emit sigRefreshAcquisition();
}

void DeviceWnd::setDeviceStateDisconnected()
{
    ui->startPusb->setEnabled(false);
    ui->stopPusb->setEnabled(false);
    ui->pausePusb->setEnabled(false);
    ui->refreshPusb->setEnabled(false);
    ui->dischargeControlPusb1->setEnabled(false);
    ui->dischargeControlPusb2->setEnabled(false);
    ui->deviceConnectedLabe->setText("Disconnected");
    ui->deviceConnectedLabe->setStyleSheet("QLabel { background-color : red; color : black; }");
}

void DeviceWnd::setDeviceStateConnected()
{
    ui->startPusb->setEnabled(true);
    ui->stopPusb->setEnabled(true);
    ui->pausePusb->setEnabled(true);
    ui->refreshPusb->setEnabled(true);
    ui->dischargeControlPusb1->setEnabled(true);
    ui->dischargeControlPusb2->setEnabled(true);
    ui->deviceConnectedLabe->setText("Connected");
    ui->deviceConnectedLabe->setStyleSheet("QLabel { background-color : green; color : black; }");
}
void    DeviceWnd::closeEvent(QCloseEvent *event)
{
    emit sigWndClosed();
}

DeviceWnd::~DeviceWnd()
{
    delete ui;
}

QPlainTextEdit *DeviceWnd::getLogWidget()
{
    return ui->loggingQpte;
}

void DeviceWnd::setDeviceState(device_state_t aDeviceState)
{
    deviceState = aDeviceState;
    switch(deviceState)
    {
    case DEVICE_STATE_UNDEFINED:
        setDeviceStateDisconnected();
        break;
    case DEVICE_STATE_CONNECTED:
        setDeviceStateConnected();
        break;
    case DEVICE_STATE_DISCONNECTED:
        setDeviceStateDisconnected();
        break;
    }
}

void DeviceWnd::printConsoleMsg(QString msg, bool exeStatus)
{
    /* call consoleWnd print Message to display recieved msg form FW <- */
    consoleWnd->printMessage(msg, exeStatus);
}

void DeviceWnd::setDeviceInterfaceSelectionState(device_interface_selection_state_t selectionState)
{
    switch(selectionState)
    {
    case DEVICE_INTERFACE_SELECTION_STATE_UNDEFINED:
        ui->adcComb->setEnabled(false);
        ui->EPControlEnableCheb->setEnabled(false);
        ui->samplingPeriodLine->setEnabled(false);
        ui->resolutionComb->setEnabled(false);
        ui->clockDivComb->setEnabled(false);
        ui->sampleTimeComb->setEnabled(false);
        ui->advanceOptionPusb->setEnabled(false);
        ui->maxNumOfPacketsLine->setEnabled(false);
        ui->saveToFileCheb->setEnabled(false);
        ui->dischargeControlPusb1->setEnabled(false);
        ui->dischargeControlPusb2->setEnabled(false);
        ui->consNamePusb->setEnabled(false);
        ui->streamServerInterfComb->setEnabled(true);
        break;
    case DEVICE_INTERFACE_SELECTION_STATE_SELECTED:
        ui->adcComb->setEnabled(true);
        ui->EPControlEnableCheb->setEnabled(true);
        ui->samplingPeriodLine->setEnabled(true);
        ui->resolutionComb->setEnabled(true);
        ui->clockDivComb->setEnabled(true);
        ui->sampleTimeComb->setEnabled(true);
        ui->advanceOptionPusb->setEnabled(true);
        ui->maxNumOfPacketsLine->setEnabled(true);
        ui->saveToFileCheb->setEnabled(true);
        ui->dischargeControlPusb1->setEnabled(true);
        ui->dischargeControlPusb2->setEnabled(true);
        ui->consNamePusb->setEnabled(true);
        ui->streamServerInterfComb->setEnabled(false);
        break;
    }
    interfaceState = selectionState;
}

void DeviceWnd::setDeviceMode(device_mode_t mode)
{
    switch(mode)
    {
    case DEVICE_MODE_EXTERNAL:
//        ui->sampleTimeComb->setEnabled(false);
//        ui->clockDivComb->setEnabled(false);
//        ui->resolutionComb->setEnabled(false);
        break;
    case DEVICE_MODE_INTERNAL:
//        ui->sampleTimeComb->setEnabled(true);
//        ui->clockDivComb->setEnabled(true);
//        ui->resolutionComb->setEnabled(true);
        break;
    }
}

bool DeviceWnd::setAdc(QString adc)
{
    if(!adcOptions->contains(adc)) return false;
    //if(!advanceConfigurationWnd->setChSampleTime(sTime)) return false;

    ui->adcComb->blockSignals(true);
    ui->adcComb->setCurrentIndex(adcOptions->indexOf(adc));
    ui->adcComb->blockSignals(false);
    if(adc == "Ext")
    {
        setDeviceMode(DEVICE_MODE_EXTERNAL);
        advanceConfigurationWnd->setDeviceMode(ADVCONFIG_ADC_MODE_EXTERNAL);
    }
    else
    {
        setDeviceMode(DEVICE_MODE_INTERNAL);
        advanceConfigurationWnd->setDeviceMode(ADVCONFIG_ADC_MODE_INTERNAL);
    }
}

QStringList *DeviceWnd::getChSamplingTimeOptions()
{
    return sampleTimeOptions;
}

QStringList *DeviceWnd::getChAvgRationOptions()
{
    return averaginOptions;
}

QStringList *DeviceWnd::getClockDivOptions()
{
    return clockDivOptions;
}

QStringList *DeviceWnd::getResolutionOptions()
{
    return resolutionOptions;
}

QStringList *DeviceWnd::getADCOptions()
{
    return adcOptions;
}

bool DeviceWnd::setChSamplingTime(QString sTime)
{
    if(!sampleTimeOptions->contains(sTime)) return false;
    if(!advanceConfigurationWnd->setChSampleTime(sTime)) return false;

    ui->sampleTimeComb->blockSignals(true);
    ui->sampleTimeComb->setCurrentIndex(sampleTimeOptions->indexOf(sTime));
    ui->sampleTimeComb->blockSignals(false);
    return true;
}

bool DeviceWnd::setChAvgRatio(QString avgRatio)
{
    if(!averaginOptions->contains(avgRatio)) return false;
    if(!advanceConfigurationWnd->setAvgRatio(avgRatio)) return false;
    return true;
}

bool DeviceWnd::setClkDiv(QString clkDiv)
{
    if(!clockDivOptions->contains(clkDiv)) return false;
    if(!advanceConfigurationWnd->setClockDiv(clkDiv)) return false;

    ui->clockDivComb->blockSignals(true);
    ui->clockDivComb->setCurrentIndex(clockDivOptions->indexOf(clkDiv));
    ui->clockDivComb->blockSignals(false);

    return true;
}

bool DeviceWnd::setResolution(QString resolution)
{
    if(!resolutionOptions->contains(resolution)) return false;
    if(!advanceConfigurationWnd->setResolution(resolution)) return false;

    ui->resolutionComb->blockSignals(true);
    ui->resolutionComb->setCurrentIndex(resolutionOptions->indexOf(resolution));
    ui->resolutionComb->blockSignals(false);
    return true;
}

bool DeviceWnd::setSamplingPeriod(QString stime)
{
    if(!advanceConfigurationWnd->setSamplingTime(stime)) return false;
    ui->samplingPeriodLine->setText(stime);
    ui->statisticsSamplingPeriodLabe2->setText(stime);
    return true;
}

bool DeviceWnd::setADCClk(QString adcClk)
{

}

bool DeviceWnd::setInCkl(QString inClk)
{
    advanceConfigurationWnd->setADCInClk(inClk);
    return true;
}

bool DeviceWnd::setCOffset(QString coffset)
{
    advanceConfigurationWnd->setCOffset(coffset);
    return true;
}

bool DeviceWnd::setVOffset(QString voffset)
{
    advanceConfigurationWnd->setVOffset(voffset);
    return true;
}

bool DeviceWnd::setLoadState(bool state)
{
    energyControlWnd->loadStatusSet(state);
    return true;
}

bool DeviceWnd::setPPathState(bool state)
{

    energyControlWnd->pPathStatusSet(state);
    return true;
}

bool DeviceWnd::setBatState(bool state)
{
    energyControlWnd->batteryStatusSet(state);
    return true;
}

bool DeviceWnd::setDACState(bool state)
{
    return true;
}

bool DeviceWnd::setChargerState(bool state)
{
    return true;
}

bool DeviceWnd::setSaveToFileState(bool state)
{
    return true;
}

void DeviceWnd::setLoadCurrentStatus(bool state)
{
    energyControlWnd->loadCurrentStatusSet(state);
}

bool DeviceWnd::setLoadCurrent(int current)
{
    energyControlWnd->loadCurrentSet(current);
    return true;
}

void DeviceWnd::setChargingCurrentStatus(bool state)
{
    energyControlWnd->chargerCurrentStatusSet(state);
}

bool DeviceWnd::setUVoltageIndication(bool state)
{
    energyControlWnd->underVoltageStatusSet(state);
    return true;
}

bool DeviceWnd::setOVoltageIndication(bool state)
{
    energyControlWnd->overVoltageStatusSet(state);
    return true;
}

bool DeviceWnd::setOCurrentIndication(bool state)
{
    energyControlWnd->overCurrentStatusSet(state);
    return true;
}

bool DeviceWnd::setChargerCurrent(int current)
{
    energyControlWnd->chargerCurrentSet(current);
    energyControlWnd->chdischChargeCurrentSet(current);
    return true;
}

bool DeviceWnd::setChargerTermCurrent(int current)
{
    energyControlWnd->chargerTermCurrentSet(current);
    return true;
}

bool DeviceWnd::setChargerTermVoltage(float voltage)
{
    energyControlWnd->chargerTermVoltageSet(voltage);
    return true;
}

bool DeviceWnd::chargingDone()
{
    energyControlWnd->chargingDone();
    return true;
}

bool DeviceWnd::setChargingStatus(QString status)
{
    if(status == "Charging")
    {
        energyControlWnd->setChargingState(Charge);
    }
    else if(status == "Discharging")
    {
        energyControlWnd->setChargingState(Discharge);
    }
    else if(status == "Idle")
    {
        energyControlWnd->setChargingState(Relax);
    }
    else
    {
        energyControlWnd->setChargingState(Unknown);
    }
}

void DeviceWnd::setStatisticsData(double dropRate, unsigned int dropPacketsNo, unsigned int fullReceivedBuffersNo, unsigned int lastBufferID)
{
    ui->statisticsPacketCounterLabe2->setText(QString::number(fullReceivedBuffersNo));
    ui->statisticsDropRateNoLabe->setText(QString::number(dropPacketsNo));
    ui->statisticsDropRateProb->setValue(dropRate);
}

void DeviceWnd::setStatisticsSamplingTime(double stime)
{
    ui->statisticsSamplingTimeLabe2->setText(QString::number(stime, 'f', 6));
}

void DeviceWnd::setStatisticsElapsedTime(int elapsedTime)
{
    int hours = elapsedTime / 3600;
    int minutes = (elapsedTime % 3600) / 60;
    int seconds = elapsedTime % 60;

    ui->statisticsAcquisitionDurationValueLabe->setText(QString("%1:%2:%3")
                                                            .arg(hours, 2, 10, QChar('0'))
                                                            .arg(minutes, 2, 10, QChar('0'))
                                                            .arg(seconds, 2, 10, QChar('0')));
}

void DeviceWnd::setConsumptionType(device_consumption_type_t actype)
{
    cType = actype;
    QRadioButton *button = qobject_cast<QRadioButton*>(measurementTypeSelection->button(1));
    switch(cType)
    {
    case DEVICE_CONSUMPTION_TYPE_CURRENT:
        consumptionChart->setTitle("Spectrum");
        consumptionChart->setXLabel("Frequency [Hz]");
        for (QAbstractButton* button : measurementTypeSelection->buttons())
        {
            button->setDisabled(false);
        }

        button->setChecked(true);
        emit sigMeasurementTypeChanged("Voltage");
        setMeasurementType(DEVICE_MEASUREMENT_TYPE_VOLTAGE);
        break;
    case DEVICE_CONSUMPTION_TYPE_CUMULATIVE:
        consumptionChart->setTitle("Consumption");
        consumptionChart->setXLabel("[ms]");
        voltageChart->setTitle("Voltage");
        voltageChart->setYLabel("[V]");
        voltageChart->setXLabel("[ms]");
        currentChart->setTitle("Current");
        currentChart->setYLabel("[mA]");
        currentChart->setXLabel("[ms]");
        for (QAbstractButton* button : measurementTypeSelection->buttons())
        {
            button->setDisabled(true);
        }
        break;
    case DEVICE_CONSUMPTION_TYPE_UNDEF:
        consumptionChart->setTitle("-");
        consumptionChart->setXLabel("-");
        break;
    }
}

void DeviceWnd::setMeasurementType(device_measurement_type_t amtype)
{
    mType = amtype;
    switch(mType)
    {
    case DEVICE_MEASUREMENT_TYPE_CURRENT:
        voltageChart->setTitle("Current");
        voltageChart->setYLabel("[mA]");
        voltageChart->setXLabel("[ms]");
        currentChart->setTitle("Current filtered");
        currentChart->setYLabel("[mA]");
        currentChart->setXLabel("[ms]");
        break;
    case DEVICE_MEASUREMENT_TYPE_VOLTAGE:
        voltageChart->setTitle("Voltage");
        voltageChart->setYLabel("[V]");
        voltageChart->setXLabel("[ms]");
        currentChart->setTitle("Voltage filtered");
        currentChart->setYLabel("[V]");
        currentChart->setXLabel("[ms]");
        break;
    case DEVICE_MEASUREMENT_TYPE_UNDEF:
        voltageChart->setTitle("-");
        currentChart->setTitle("-");
        break;
    }
}

bool DeviceWnd::plotVoltageValues(QVector<double> values, QVector<double> keys)
{
    switch(cType)
    {
    case DEVICE_CONSUMPTION_TYPE_CURRENT:
        voltageChart->setData(values, keys);
        break;
    case DEVICE_CONSUMPTION_TYPE_CUMULATIVE:
        voltageChart->appendData(values, keys);
        break;
    case DEVICE_CONSUMPTION_TYPE_UNDEF:
        break;
    }
    return true;
}

bool DeviceWnd::plotCurrentValues(QVector<double> values, QVector<double> keys)
{
    switch(cType)
    {
    case DEVICE_CONSUMPTION_TYPE_CURRENT:
        currentChart->setData(values, keys);
        break;
    case DEVICE_CONSUMPTION_TYPE_CUMULATIVE:
        currentChart->appendData(values, keys);
        break;
    case DEVICE_CONSUMPTION_TYPE_UNDEF:
        break;
    }
    return true;
}

bool DeviceWnd::plotConsumptionValues(QVector<double> values, QVector<double> keys)
{
    switch(cType)
    {
    case DEVICE_CONSUMPTION_TYPE_CURRENT:
        consumptionChart->setData(values, keys);
        break;
    case DEVICE_CONSUMPTION_TYPE_CUMULATIVE:
        consumptionChart->appendData(values, keys);
        break;
    case DEVICE_CONSUMPTION_TYPE_UNDEF:
        break;
    }
    return true;
}

bool DeviceWnd::plotConsumptionEBP(QVector<double> values, QVector<double> keys)
{
    if(values.size() > 0)
    {
        consumptionChart->scatterAddData(values, keys);
    }
    return true;
}

bool DeviceWnd::plotConsumptionEBPWithName(double value, double key, QString name)
{
    consumptionChart->scatterAddDataWithName(value, key, name);
    currentChart->scatterAddDataWithName(value, key, name);
    voltageChart->scatterAddDataWithName(value, key, name);
    return true;
}

bool DeviceWnd::showStatistic(device_stat_info statInfo)
{
    dataAnalyzer->setVoltageStatisticInfo(statInfo.voltageAvg, statInfo.voltageMax, statInfo.voltageMin);
    dataAnalyzer->setCurrentStatisticInfo(statInfo.currentAvg, statInfo.currentMax, statInfo.currentMin);
    dataAnalyzer->setConsumptionStatisticInfo(statInfo.consumptionAvg, statInfo.consumptionMax, statInfo.consumptionMin);
    return true;
}

bool DeviceWnd::setWorkingSpaceDir(QString aWsPath)
{
    wsPath = aWsPath;
    ui->consNameLine->setText(wsPath);
    return true;
}

void DeviceWnd::setCalibrationData(CalibrationData *data)
{
    calibrationWnd->setCalibrationData(data);
}
