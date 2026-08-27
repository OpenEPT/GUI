#include "devicewnd.h"
#include "ui_devicewnd.h"
#include "Windows/Console/consolewnd.h"
#include <QFileDialog>
#include <QNetworkInterface>
#include <QOpenGLWidget>
//#include <QDebug>

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

    ui->maxNumOfPacketsLine->setText("");
    ui->samplesNoLine->setText("");
    ui->statisticsPacketCounterLabe2->setText(QString::number(0));
    ui->statisticsDropRateProb->setValue(0);
    ui->statisticsSamplingPeriodLabe2->setText(QString::number(0));
    ui->samplingPeriodLine->setText(QString::number(0));



    setStatisticsElapsedTime(0);


    setDeviceNetworkState(DEVICE_STATE_UNDEFINED);

    consoleWnd  = new ConsoleWnd();

    calibrationWnd = new CalibrationWnd();

    ui->GraphicsTopHorl->addWidget(voltageChart);
    ui->GraphicsTopHorl->addWidget(currentChart);
    ui->GraphicsBottomVerl->addWidget(consumptionChart, Qt::AlignCenter);

    connect(ui->saveToFileCheb, SIGNAL(stateChanged(int)), this, SLOT(onSaveToFileChanged(int)));
    connect(ui->EPControlEnableCheb, SIGNAL(stateChanged(int)), this, SLOT(onEPEnableChanged(int)));
    connect(ui->consNamePusb, SIGNAL(clicked(bool)), this, SLOT(onSetConsumptionName()));
    connect(ui->startPusb, SIGNAL(clicked(bool)), this, SLOT(onStartAcquisition()));
    connect(ui->pausePusb, SIGNAL(clicked(bool)), this, SLOT(onPauseAcquisition()));
    connect(ui->stopPusb, SIGNAL(clicked(bool)), this, SLOT(onStopAcquisiton()));
    connect(ui->refreshPusb, SIGNAL(clicked(bool)), this, SLOT(onRefreshAcquisiton()));
    connect(ui->ConsolePusb, SIGNAL(clicked(bool)), this, SLOT(onConsolePressed()));
    connect(ui->DataStatisticsPushb, SIGNAL(clicked(bool)), this, SLOT(onDataAnalyzerPressed()));

    connect(ui->samplingPeriodLine,     SIGNAL(returnPressed()),                    this, SLOT(onSamplingPeriodChanged()));
    connect(ui->streamServerInterfComb, SIGNAL(currentTextChanged(QString)),        this, SLOT(onInterfaceChanged(QString)));
    connect(ui->maxNumOfPacketsLine,    SIGNAL(editingFinished()),                  this, SLOT(onMaxNumberOfBuffersChanged()));
     connect(ui->samplesNoLine,          SIGNAL(returnPressed()),                    this, SLOT(onSamplesNoChanged()));



    connect(consoleWnd, SIGNAL(sigControlMsgSend(QString)), this, SLOT(onNewControlMsgRcvd(QString)));

    connect(calibrationWnd, SIGNAL(sigCalibrationDataUpdated()), this, SLOT(onCalibrationUpdated()));
    connect(calibrationWnd, SIGNAL(sigCalibrationStoreRequest()), this, SLOT(onCalibrationStoreRequest()));

    connect(ui->dischargeControlPusb1, SIGNAL(clicked(bool)), this, SLOT(onCalibrationButtonPressed(bool)));
    connect(ui->dischargeControlPusb2, SIGNAL(clicked(bool)), this, SLOT(onEnenergyControlButtonPressed(bool)));


    setDeviceInterfaceSelectionState(DEVICE_INTERFACE_SELECTION_STATE_UNDEFINED);
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

void DeviceWnd::onCalibrationStoreRequest()
{
    emit sigCalibrationStoreRequest();
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
    m_param->setParamValue("maxNumberOfBuffers", maxNumberOfSamplesBuffers);
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


void DeviceWnd::onSamplingPeriodChanged()
{
    QString time = ui->samplingPeriodLine->text();
    m_param->setParamValue("samplingPeriod", time);
    ui->statisticsSamplingPeriodLabe2->setText(time);
    emit sigSamplingPeriodChanged(time);
}

void DeviceWnd::onSamplesNoChanged()
{
    unsigned int samplesNo = ui->samplesNoLine->text().toUInt();
    if(samplesNo > 250)
    {
        samplesNo = 250;
        ui->samplesNoLine->setText(QString::number(250));
    }
    if(samplesNo < 1)
    {
        samplesNo = 1;
        ui->samplesNoLine->setText(QString::number(1));
    }
    //advanceConfigurationWnd->setSamplingTime(time);
    //ui->statisticsSamplingPeriodLabe2->setText(time);
    m_param->setParamValue("streamPacketSize", QString::number(samplesNo));
    emit sigSamplesNoChanged(samplesNo);
}


void    DeviceWnd::onAdvanceConfigurationButtonPressed(bool pressed)
{
    configurationWnd->show();
    configurationWnd->raise();
    configurationWnd->activateWindow();
}

void DeviceWnd::onCalibrationButtonPressed(bool pressed)
{
    calibrationWnd->showWnd();
}

void DeviceWnd::onEnenergyControlButtonPressed(bool pressed)
{
    energyControlWnd->show();
}

void DeviceWnd::onSaveToFileChanged(int value)
{
    if(value == Qt::Checked)
    {
        const QString defaultName =
            QDateTime::currentDateTime().toString("yyyy_MM_dd_HH_mm");

        ui->consNamePusb->setEnabled(true);
        ui->consNameLab->setEnabled(true);
        ui->consNameLine->setEnabled(true);
        ui->consNameLine->setPlaceholderText("Set Consumption Name");

        if(ui->consNameLine->text().isEmpty())
        {
            ui->consNameLine->setText(defaultName);
        }

        emit sigSaveToFileEnabled(true);
    }
    else
    {
        ui->consNamePusb->setEnabled(false);
        ui->consNameLab->setEnabled(false);
        ui->consNameLine->setEnabled(false);
        ui->consNameLine->clear();
        ui->consNameLine->setPlaceholderText("Enable \"Save to file\"");

        emit sigSaveToFileEnabled(false);
    }
}

void DeviceWnd::onChDschSaveToFileChanged(bool state)
{
    emit sigChDschSaveToFileToggled(state);
}

void DeviceWnd::onConfWndGetBDContent()
{
    emit sigReadFullBDContent();
}

void DeviceWnd::onConfWndSetBDContent(QByteArray content)
{
    emit sigSetBDContent(content);
}

void DeviceWnd::onConfWndBDFormat()
{
    emit sigBDFormat();
}

void DeviceWnd::onChargerConfWndGetBDContent()
{
    emit sigChargerReadFullBDContent();
}

void DeviceWnd::onChargerConfWndSetBDContent(QByteArray content)
{
    emit sigChargerSetBDContent(content);
}

void DeviceWnd::onChargerConfWndBDFormat()
{
    emit sigChargerBDFormat();
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
    configurationWnd->close();
    consoleWnd->close();
    energyControlWnd->close();
    emit sigWndClosed();
}

void DeviceWnd::onDeviceConfigSet(QMap<QString, QString> changedFields)
{
    emit sigDeviceConfigSet(changedFields);
}

void DeviceWnd::onDeviceConfigGet()
{
    emit sigDeviceConfigGet();
}

void DeviceWnd::onDeviceConfigStore()
{
    emit sigDeviceConfigStore();
}

void DeviceWnd::onDeviceReset()
{
    emit sigDeviceReset();
}


DeviceWnd::~DeviceWnd()
{
    delete ui;
}

QPlainTextEdit *DeviceWnd::getLogWidget()
{
    return ui->loggingQpte;
}

void DeviceWnd::setParameters(DeviceParameters *params)
{
    configurationWnd = new ConfigurationWnd(0, params);
    m_param = params;
    ui->maxNumOfPacketsLine->setText(params->getParamValue("maxNumberOfBuffers"));
    ui->samplesNoLine->setText(params->getParamValue("streamPacketSize"));

    connect(configurationWnd,
            &ConfigurationWnd::sigDeviceConfigSet,
            this,
            &DeviceWnd::onDeviceConfigSet);

    connect(configurationWnd,
            &ConfigurationWnd::sigDeviceConfigAcquireRequest,
            this,
            &DeviceWnd::onDeviceConfigGet);

    connect(configurationWnd,
            &ConfigurationWnd::sigResetDevice,
            this,
            &DeviceWnd::onDeviceReset);

    connect(configurationWnd,
            &ConfigurationWnd::sigDeviceConfigStore,
            this,
            &DeviceWnd::onDeviceConfigStore);

    connect(configurationWnd,
            &ConfigurationWnd::sigBDContentGetRequest,
            this,
            &DeviceWnd::onConfWndGetBDContent);

    connect(configurationWnd,
            &ConfigurationWnd::sigBDContentSetRequest,
            this,
            &DeviceWnd::onConfWndSetBDContent);

    connect(configurationWnd,
            &ConfigurationWnd::sigBDFormatRequest,
            this,
            &DeviceWnd::onConfWndBDFormat);

    connect(configurationWnd,
            &ConfigurationWnd::sigChargerBDContentGetRequest,
            this,
            &DeviceWnd::onChargerConfWndGetBDContent);

    connect(configurationWnd,
            &ConfigurationWnd::sigChargerBDContentSetRequest,
            this,
            &DeviceWnd::onChargerConfWndSetBDContent);

    connect(configurationWnd,
            &ConfigurationWnd::sigChargerBDFormatRequest,
            this,
            &DeviceWnd::onChargerConfWndBDFormat);


    connect(this, &QObject::destroyed,
            configurationWnd, &QObject::deleteLater);


}

void DeviceWnd::setDeviceNetworkState(device_state_t aDeviceState)
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

void DeviceWnd::setDeviceAcqState(device_acq_mode_t aAcqState)
{
    acqState = aAcqState;
    switch(acqState)
    {
    case DEVICE_ACQ_ACTIVE:
        ui->EPControlEnableCheb->setEnabled(false);
        ui->samplingPeriodLine->setEnabled(false);
        ui->advanceOptionPusb->setEnabled(false);
        ui->maxNumOfPacketsLine->setEnabled(false);
        ui->ConsolePusb->setEnabled(true);
        ui->DataStatisticsPushb->setEnabled(true);
        ui->saveToFileCheb->setEnabled(false);
        ui->dischargeControlPusb1->setEnabled(true);
        ui->dischargeControlPusb2->setEnabled(true);
        ui->consNamePusb->setEnabled(true);
        ui->samplesNoLine->setEnabled(false);
        ui->streamServerInterfComb->setEnabled(false);
        ui->startPusb->setEnabled(false);
        ui->stopPusb->setEnabled(true);
        ui->pausePusb->setEnabled(true);
        break;
    case DEVICE_ACQ_PAUSE:
    case DEVICE_ACQ_UNDEFINED:
        ui->EPControlEnableCheb->setEnabled(true);
        ui->samplingPeriodLine->setEnabled(true);
        ui->advanceOptionPusb->setEnabled(true);
        ui->maxNumOfPacketsLine->setEnabled(true);
        ui->ConsolePusb->setEnabled(true);
        ui->DataStatisticsPushb->setEnabled(true);
        ui->saveToFileCheb->setEnabled(true);
        ui->dischargeControlPusb1->setEnabled(true);
        ui->dischargeControlPusb2->setEnabled(true);
        ui->consNamePusb->setEnabled(true);
        ui->samplesNoLine->setEnabled(true);
        ui->streamServerInterfComb->setEnabled(false);
        ui->startPusb->setEnabled(true);
        ui->stopPusb->setEnabled(true);
        ui->pausePusb->setEnabled(false);
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
        ui->EPControlEnableCheb->setEnabled(false);
        ui->samplingPeriodLine->setEnabled(false);
        ui->advanceOptionPusb->setEnabled(false);
        ui->maxNumOfPacketsLine->setEnabled(false);
        ui->ConsolePusb->setEnabled(false);
        ui->DataStatisticsPushb->setEnabled(false);
        ui->saveToFileCheb->setEnabled(false);
        ui->dischargeControlPusb1->setEnabled(false);
        ui->dischargeControlPusb2->setEnabled(false);
        ui->consNamePusb->setEnabled(false);
        ui->samplesNoLine->setEnabled(false);
        ui->streamServerInterfComb->setEnabled(true);
        ui->startPusb->setEnabled(false);
        ui->pausePusb->setEnabled(false);
        ui->stopPusb->setEnabled(false);
        ui->refreshPusb->setEnabled(false);
        break;
    case DEVICE_INTERFACE_SELECTION_STATE_SELECTED:
        ui->EPControlEnableCheb->setEnabled(true);
        ui->samplingPeriodLine->setEnabled(true);
        ui->samplesNoLine->setEnabled(true);
        ui->ConsolePusb->setEnabled(true);
        ui->DataStatisticsPushb->setEnabled(true);
        ui->advanceOptionPusb->setEnabled(true);
        ui->maxNumOfPacketsLine->setEnabled(true);
        ui->saveToFileCheb->setEnabled(true);
        ui->dischargeControlPusb1->setEnabled(true);
        ui->dischargeControlPusb2->setEnabled(true);
        ui->consNamePusb->setEnabled(true);
        ui->streamServerInterfComb->setEnabled(false);
        ui->startPusb->setEnabled(true);
        ui->pausePusb->setEnabled(true);
        ui->stopPusb->setEnabled(true);
        ui->refreshPusb->setEnabled(true);
        break;
    }
    interfaceState = selectionState;
}




bool DeviceWnd::setSamplingPeriod(QString stime)
{
    configurationWnd->setParamValue("samplingPeriod", stime);
    ui->samplingPeriodLine->setText(stime);
    ui->statisticsSamplingPeriodLabe2->setText(stime);
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

void DeviceWnd::setChargerConnectionStatus(bool status)
{
    energyControlWnd->chargerConnectionStatusSet(status);
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

bool DeviceWnd::setUVoltageValue(float value)
{

    configurationWnd->setParamValue("underVoltageValue", QString::number(value));
    return true;
}

bool DeviceWnd::setOVoltageValue(float value)
{
    configurationWnd->setParamValue("overVoltageValue", QString::number(value));
    return true;
}

bool DeviceWnd::setOCurrentValue(int value)
{
    configurationWnd->setParamValue("overCurrentValue", QString::number(value));
    return true;
}


bool DeviceWnd::setBDSize(int value)
{
    configurationWnd->setParamValue("bdSize", QString::number(value));
    return true;
}

bool DeviceWnd::setBDContent(QString content)
{
    configurationWnd->setBDContent(content);
    return true;
}

bool DeviceWnd::setChargerBDContent(QString content)
{
    configurationWnd->setChargerBDContent(content);
    return true;
}

bool DeviceWnd::setChargerCurrent(int current)
{
    energyControlWnd->chargerCurrentSet(current);
    energyControlWnd->chdischChargeCurrentSet(current);

    configurationWnd->setParamValue("chargerChargeCurrent", QString::number(current));
    return true;
}

bool DeviceWnd::setChargerTermCurrent(int current)
{
    int currentPercent = 0;

    switch(current)
    {
        case 0:
            currentPercent = 0;
            break;

        case 1:
            currentPercent = 5;
            break;

        case 2:
            currentPercent = 10;
            break;

        case 3:
            currentPercent = 20;
            break;

        default:
            return false;
    }

    energyControlWnd->chargerTermCurrentSet(currentPercent);
    configurationWnd->setParamValue("chargerTermCurrent", QString::number(currentPercent));

    return true;
}

bool DeviceWnd::setChargerTermVoltage(float voltage)
{
    energyControlWnd->chargerTermVoltageSet(voltage);

    configurationWnd->setParamValue("chargerTermVoltage", QString::number(voltage));
    return true;
}

bool DeviceWnd::setChargerMaxCurrent(int current)
{
    int currentMa = 0;

    switch(current)
    {
        case 0:
            currentMa = 50;
            break;

        case 1:
            currentMa = 100;
            break;

        case 2:
            currentMa = 200;
            break;

        case 3:
            currentMa = 300;
            break;

        case 4:
            currentMa = 400;
            break;

        case 5:
            currentMa = 500;
            break;

        case 6:
            currentMa = 700;
            break;

        case 7:
            currentMa = 1100;
            break;

        default:
            return false;
    }

    configurationWnd->setParamValue("chargerMaxChargeCurrent", QString::number(currentMa));

    energyControlWnd->chargerMaxCurrentSet(currentMa);

    return true;
}

bool DeviceWnd::setChargerHWSerial(QString serial)
{
    configurationWnd->setParamValue("chargerSerial", serial);
    energyControlWnd->chargerFWVersionSet(serial);
    return true;
}

bool DeviceWnd::setChargerFWSerial(QString serial)
{
    configurationWnd->setParamValue("chargerFwVersion", serial);
    energyControlWnd->chargerSerialNumberSet(serial);
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
        energyControlWnd->setChargingState(Unknown1);
    }
    return true;
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

void DeviceWnd::setConfigurationAppliedStatus(bool status)
{
    configurationWnd->setConfigurationAppliedStatus(status);
}

void DeviceWnd::setConfigurationBDProgressStatus(int percentage, QString status)
{
    configurationWnd->setBDProgress(percentage, status);
}

void DeviceWnd::setConfigurationChargerBDProgressStatus(int percentage, QString status)
{
    configurationWnd->setChargerBDProgress(percentage, status);
}



bool DeviceWnd::plotVoltageValues(QVector<double> values, QVector<double> keys)
{
    voltageChart->appendData(values, keys);
    return true;
}

bool DeviceWnd::plotCurrentValues(QVector<double> values, QVector<double> keys)
{
    currentChart->appendData(values, keys);
    return true;
}

bool DeviceWnd::plotConsumptionValues(QVector<double> values, QVector<double> keys)
{
    consumptionChart->appendData(values, keys);
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
