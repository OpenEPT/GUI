#include <QMessageBox>
#include "Processing/batteryparamsextraction.h"

#define DEVICECONTAINER_RELAX_SAMPLES_MAX      2000000
#define DEVICECONTAINER_RELAX_CHECK_PERIOD     500.0
#include <devicecontainer.h>
#include "devicecontainer.h"

DeviceContainer::DeviceContainer(QObject *parent,
                                 DeviceWnd* aDeviceWnd,
                                 Device* aDevice,
                                 ApplicationParameters* appParam,
                                 QDockWidget* dock)
    : QObject{parent}
{
    deviceWnd       = aDeviceWnd;
    device          = aDevice;
    log             = new Log();
    fileProcessing  = new FileProcessing();
    batteryParamsExtraction = new BatteryParamsExtraction(this);
    connect(batteryParamsExtraction, &BatteryParamsExtraction::sigCycleFinished,
            this, &DeviceContainer::onBatParamCycleFinished);
    samplingPeriodMs = 0;
    uVoltageActive = false;
    oVoltageActive = false;
    oCurrentActive = false;
    batParamRelaxEnabled = false;
    batParamRelaxThreshold = 0;
    batParamRelaxWindow = 0;
    batParamRelaxFilter = 0;
    batParamAverageSum = 0;
    batParamAverageFirst = 0;
    batParamRelaxPassesDone = 0;
    batParamRelaxPauseActive = false;
    batParamRelaxRunning = false;
    batParamRelaxRestarting = false;
    batteryParamsWnd = NULL;
    qRegisterMetaType<batteryparams_cycle_t>("batteryparams_cycle_t");
    log->assignLogWidget(deviceWnd->getLogWidget());
    connect(deviceWnd->getLogDock(), SIGNAL(sigFilterChanged(int)), log, SLOT(setFilter(int)));
    connect(deviceWnd->getLogDock(), SIGNAL(sigClearRequested()), log, SLOT(clear()));
    connect(deviceWnd->getLogDock(), SIGNAL(sigFollowOutputChanged(bool)), log, SLOT(setFollowOutput(bool)));
    log->setFollowOutput(deviceWnd->getLogDock()->getFollowOutput());
    m_AppParamsRef  = appParam;
    consumptionProfileName = "";
    consumptionProfileNameSet = false;
    consumptionProfileNameExists = false;
    globalSaveToFileEnabled   = false;
    epEnabled           = false;
    m_dock              = dock;

    elapsedTime     = 0;
    timer           = new QTimer();

    connect(timer,      SIGNAL(timeout()),                                           this, SLOT(onTimeout()));


    /*Device window signals*/
    connect(deviceWnd,  SIGNAL(sigWndClosed()),                                     this, SLOT(onDeviceWndClosed()));
    connect(deviceWnd,  SIGNAL(sigSaveToFileEnabled(bool)),                         this, SLOT(onDeviceWndSaveToFileChanged(bool)));
    connect(deviceWnd,  SIGNAL(sigEPEnable(bool)),                                  this, SLOT(onDeviceWndEPEnable(bool)));
    connect(deviceWnd,  SIGNAL(sigNewControlMessageRcvd(QString)),                  this, SLOT(onConsoleWndMessageRcvd(QString)));
    connect(deviceWnd,  SIGNAL(sigSamplesNoChanged(unsigned int)),                  this, SLOT(onDeviceWndSamplesNoChanged(unsigned int)));
    connect(deviceWnd,  SIGNAL(sigSamplingPeriodChanged(QString)),                  this, SLOT(onDeviceWndSamplingPeriodChanged(QString)));
    connect(deviceWnd,  SIGNAL(sigNewInterfaceSelected(QString)),                   this, SLOT(onDeviceWndInterfaceChanged(QString)));
    connect(deviceWnd,  SIGNAL(sigStartAcquisition()),                              this, SLOT(onDeviceWndAcquisitionStart()));
    connect(deviceWnd,  SIGNAL(sigStopAcquisition()),                               this, SLOT(onDeviceWndAcquisitionStop()));
    connect(deviceWnd,  SIGNAL(sigPauseAcquisition()),                              this, SLOT(onDeviceWndAcquisitionPause()));
    connect(deviceWnd,  SIGNAL(sigRefreshAcquisition()),                            this, SLOT(onDeviceWndAcquisitionRefresh()));
    connect(deviceWnd,  SIGNAL(sigMaxNumberOfBuffersChanged(uint)),                 this, SLOT(onDeviceWndMaxNumberOfBuffersChanged(uint)));
    connect(deviceWnd,  SIGNAL(sigConsumptionTypeChanged(QString)),                 this, SLOT(onDeviceWndConsumptionTypeChanged(QString)));
    connect(deviceWnd,  SIGNAL(sigMeasurementTypeChanged(QString)),                 this, SLOT(onDeviceWndMeasurementTypeChanged(QString)));
    connect(deviceWnd,  SIGNAL(sigConsumptionProfileNameChanged(QString)),          this, SLOT(onDeviceWndConsumptionProfileNameChanged(QString)));
    connect(deviceWnd,  SIGNAL(sigCalibrationUpdated()),                            this, SLOT(onDeviceWndCalibrationUpdated()));
    connect(deviceWnd,  SIGNAL(sigCalibrationStoreRequest()),                       this, SLOT(onDeviceWndCalibrationStoreRequest()));
    connect(deviceWnd,  SIGNAL(sigAutoCalApplyCalibration()),                       this, SLOT(onDeviceWndAutoCalApply()));
    connect(deviceWnd,  SIGNAL(sigAutoCalSetLoadCurrent(int)),                      this, SLOT(onDeviceWndAutoCalSetLoadCurrent(int)));
    connect(deviceWnd,  SIGNAL(sigAutoCalSetLoadEnabled(bool)),                     this, SLOT(onDeviceWndAutoCalSetLoadEnabled(bool)));
    connect(deviceWnd,  SIGNAL(sigAutoCalResetProtection()),                        this, SLOT(onDeviceWndAutoCalResetProtection()));

    connect(deviceWnd,  SIGNAL(sigLoadStatusChanged(bool)),                         this, SLOT(onDeviceWndLoadStatusChanged(bool)));
    connect(deviceWnd,  SIGNAL(sigPPathStatusChanged(bool)),                        this, SLOT(onDeviceWndPPathStatusChanged(bool)));
    connect(deviceWnd,  SIGNAL(sigBatteryStatusChanged(bool)),                      this, SLOT(onDeviceWndBatteryStatusChanged(bool)));
    connect(deviceWnd,  SIGNAL(sigResetProtection()),                               this, SLOT(onDeviceWndResetProtection()));
    connect(deviceWnd,  SIGNAL(sigLoadCurrentStatusChanged(bool)),                  this, SLOT(onDeviceWndLoadCurrentSetStatus(bool)));
    connect(deviceWnd,  SIGNAL(sigLoadCurrentChanged(unsigned int)),                this, SLOT(onDeviceWndLoadCurrentSetValue(unsigned int)));
    connect(deviceWnd,  &DeviceWnd::sigLoadWaveChanged,                              this, &DeviceContainer::onDeviceWndLoadWaveSet);
    connect(deviceWnd,  &DeviceWnd::sigLoadWaveStatusChanged,                        this, &DeviceContainer::onDeviceWndLoadWaveSetStatus);
    connect(deviceWnd,  &DeviceWnd::sigBatParamViewRequested,                        this, &DeviceContainer::onDeviceWndBatParamViewRequested);
    connect(deviceWnd,  &DeviceWnd::sigBatParamCapacityChanged,                      this, &DeviceContainer::onDeviceWndBatParamCapacityChanged);
    connect(deviceWnd,  &DeviceWnd::sigBatParamRelaxationChanged,                    this, &DeviceContainer::onDeviceWndBatParamRelaxationChanged);
    connect(deviceWnd,  &DeviceWnd::sigLoadWaveClear,                                this, &DeviceContainer::onDeviceWndLoadWaveClear);
    connect(deviceWnd,  SIGNAL(sigChargingCurrentStatusChanged(bool)),              this, SLOT(onDeviceWndChargingCurrentSetStatus(bool)));
    connect(deviceWnd,  SIGNAL(sigChargingCurrentChanged(unsigned int)),            this, SLOT(onDeviceWndChargingCurrentSetValue(unsigned int)));
    connect(deviceWnd,  SIGNAL(sigChargingTermCurrentChanged(unsigned int)),        this, SLOT(onDeviceWndChargingTermCurrentSetValue(unsigned int)));
    connect(deviceWnd,  SIGNAL(sigChargingTermVoltageChanged(float)),               this, SLOT(onDeviceWndChargingTermVoltageSetValue(float)));
    connect(deviceWnd,  SIGNAL(sigChDschSaveToFileToggled(bool)),                   this, SLOT(onDeviceWndChDschSaveToFileChanged(bool)));

    /*Device signals*/
    connect(device,     SIGNAL(sigControlLinkConnected()),                          this, SLOT(onDeviceControlLinkConnected()));
    connect(device,     SIGNAL(sigControlLinkDisconnected()),                       this, SLOT(onDeviceControlLinkDisconnected()));
    connect(device,     SIGNAL(sigStatusLinkNewDeviceAdded(QString)),               this, SLOT(onDeviceStatusLinkNewDeviceAdded(QString)));
    connect(device,     SIGNAL(sigStatusLinkNewMessageReceived(QString,QString)),   this, SLOT(onDeviceStatusLinkNewMessageReceived(QString,QString)));
    connect(device,     SIGNAL(sigNewResponseReceived(QString, bool)),              this, SLOT(onDeviceHandleControlMsgResponse(QString, bool)));
    connect(device,     SIGNAL(sigSampleTimeObtained(QString)),                     this, SLOT(onDeviceSamplingPeriodObtained(QString)));
    connect(device,     SIGNAL(sigSamplingTimeChanged(double)),                     this, SLOT(onDeviceSamplingTimeChanged(double)));
    connect(device,     SIGNAL(sigAcqusitionStarted()),                             this, SLOT(onDeviceAcquisitonStarted()));
    connect(device,     SIGNAL(sigAcqusitionStopped()),                             this, SLOT(onDeviceAcquisitonStopped()));

    connect(device,     SIGNAL(sigPPathStateObtained(bool)),                        this,  SLOT(onDevicePPathStateObtained(bool)));
    connect(device,     SIGNAL(sigBatStateObtained(bool)),                          this,  SLOT(onDeviceBatStateObtained(bool)));
    connect(device,     SIGNAL(sigLoadStateObtained(bool)),                         this,  SLOT(onDeviceLoadStateObtained(bool)));
    connect(device,     SIGNAL(sigLoadCurrentObtained(int )),                       this,  SLOT(onDeviceLoadCurrentObtained(int)));
    connect(device,     SIGNAL(sigDACStateObtained(bool)),                          this,  SLOT(onDeviceDACStateObtained(bool)));
    connect(device,     SIGNAL(sigOVoltageObtained(bool)),                          this,  SLOT(onDeviceOVoltageObtained(bool)));
    connect(device,     SIGNAL(sigUVoltageObtained(bool)),                          this,  SLOT(onDeviceUVoltageObtained(bool)));
    connect(device,     SIGNAL(sigOCurrentObtained(bool)),                          this,  SLOT(onDeviceOCurrentObtained(bool)));
    connect(device,     SIGNAL(sigChargerConnectionStatusObtained(bool)),           this,  SLOT(onDeviceChargerConnectionStatusOntained(bool)));

    connect(device,     SIGNAL(sigChargerCurrentObtained(int )),                    this,  SLOT(onDeviceChargerCurrentObtained(int)));
    connect(device,     SIGNAL(sigChargerTermCurrentObtained(int )),                this,  SLOT(onDeviceChargerTermCurrentObtained(int)));
    connect(device,     SIGNAL(sigChargerTermVoltageObtained(float )),              this,  SLOT(onDeviceChargerTermVoltageObtained(float)));
    connect(device,     SIGNAL(sigChargerMaxChargingCurrentObtained(int )),         this,  SLOT(onDeviceChargerMaxCurrentObtained(int)));
    connect(device,     SIGNAL(sigChargingDone()),                                  this,  SLOT(onDeviceChargingDone()));
    connect(device,     &Device::sigLoadWaveStopped,                                this,  &DeviceContainer::onDeviceLoadWaveStopped);


    connect(device,     SIGNAL(sigChargerHWSerialObtained(QString)),                this,  SLOT(onDeviceChargerHWSerialObtained(QString)));
    connect(device,     SIGNAL(sigChargerFWVersionObtained(QString)),                this,  SLOT(onDeviceChargerFWVersionObtained(QString)));

    connect(device,     SIGNAL(sigVoltageCurrentSamplesReceived(QVector<double>,QVector<double>,QVector<double>, QVector<double>)),
            this, SLOT(onDeviceNewVoltageCurrentSamplesReceived(QVector<double>,QVector<double>,QVector<double>, QVector<double>)));
    connect(device,     SIGNAL(sigNewSamplesBuffersProcessingStatistics(double,uint,uint,uint, unsigned short)), this, SLOT(onDeviceNewSamplesBuffersProcessingStatistics(double,uint,uint,uint, unsigned short)));
    connect(device,     SIGNAL(sigNewConsumptionDataReceived(QVector<double>,QVector<double>, dataprocessing_consumption_mode_t)),
            this, SLOT(onDeviceNewConsumptionDataReceived(QVector<double>,QVector<double>, dataprocessing_consumption_mode_t)));
    connect(device,     SIGNAL(sigNewEBP(QVector<double>,QVector<double>)), this, SLOT(onDeviceNewEBP(QVector<double>,QVector<double>)));
    connect(device,     SIGNAL(sigNewEBPFull(double,double,QString)), this, SLOT(onDeviceNewEBPFull(double,double,QString)));
    connect(device,     SIGNAL(sigNewStatisticsReceived(dataprocessing_dev_info_t,dataprocessing_dev_info_t,dataprocessing_dev_info_t)),
            this, SLOT(onDeviceNewStatisticsReceived(dataprocessing_dev_info_t,dataprocessing_dev_info_t,dataprocessing_dev_info_t)));

    connect(device,     SIGNAL(sigChargingStatusChanged(charginganalysis_status_t)),
            this, SLOT(onDeviceMeasurementEnergyFlowStatusChanged(charginganalysis_status_t)));

    connect(device, SIGNAL(sigUVoltageValueObtained(float)),
            this, SLOT(onDeviceUVoltageValueObtained(float)));

    connect(device, SIGNAL(sigOVoltageValueObtained(float)),
            this, SLOT(onDeviceOVoltageValueObtained(float)));

    connect(device, SIGNAL(sigOCurrentValueObtained(int)),
            this, SLOT(onDeviceOCurrentValueObtained(int)));


    connect(device, SIGNAL(sigBDSizeObtained(int)),
            this, SLOT(onDeviceBDSizeObtained (int)));

    connect(deviceWnd, SIGNAL(sigReadFullBDContent()),
            this, SLOT(onDeviceWndGetBDContent()));

    connect(deviceWnd, SIGNAL(sigSetBDContent(QByteArray)),
            this, SLOT(onDeviceWndSetBDContent(QByteArray)));

    connect(device, SIGNAL(sigBDChunkRead(float)), this, SLOT(onDeviceBDChunkRead(float)));
    connect(device, SIGNAL(sigBDChunkWrite(float)), this, SLOT(onDeviceBDChunkWrite(float)));


    connect(deviceWnd, SIGNAL(sigBDFormat()),
            this, SLOT(onDeviceWndBDFormat()));


    connect(deviceWnd, SIGNAL(sigChargerReadFullBDContent()),
            this, SLOT(onDeviceWndChargerGetBDContent()));


    connect(deviceWnd, SIGNAL(sigChargerSetBDContent(QByteArray)),
            this, SLOT(onDeviceWndChargerSetBDContent(QByteArray)));



    deviceWnd->setCalibrationData(device->getCalibrationData());

    log->printLogMessage("Device container successfully created", LOG_MESSAGE_TYPE_INFO);
    device->statusLinkServerCreate();
    device->epLinkServerCreate();
    fillDeviceSetFunctions();

    connect(deviceWnd,
            &DeviceWnd::sigDeviceConfigSet,
            this,
            &DeviceContainer::onDeviceConfigUpdate);

    connect(deviceWnd,
            &DeviceWnd::sigDeviceConfigGet,
            this,
            &DeviceContainer::onDeviceConfigGet);
    connect(deviceWnd,
            &DeviceWnd::sigDeviceConfigStore,
            this,
            &DeviceContainer::onDeviceConfigStore);

    connect(deviceWnd,
            &DeviceWnd::sigDeviceReset,
            this,
            &DeviceContainer::onDeviceReset);

}
void DeviceContainer::fillDeviceSetFunctions()
{
    auto params = device->parameters();

    /**************************************************************
     * PROTECTION
     **************************************************************/

    {
        auto &p = params->getParamRef("underVoltageValue");
        p.setFn = [this](const QVariant& v){
            return device->setUVoltageValue(v.toFloat());
        };
        p.getFn = [this](){
            device->getUVoltageValue();
        };
    }

    {
        auto &p = params->getParamRef("overVoltageValue");
        p.setFn = [this](const QVariant& v){
            return device->setOVoltageValue(v.toFloat());
        };
        p.getFn = [this](){
            device->getOVoltageValue();
        };
    }

    {
        auto &p = params->getParamRef("overCurrentValue");
        p.setFn = [this](const QVariant& v){
            return device->setOCurrentValue(v.toInt());
        };
        p.getFn = [this](){
            device->getOCurrentValue();
        };
    }

    {
        auto &p = params->getParamRef("bdSize");
        p.setFn = nullptr;
        p.getFn = [this](){
            device->getBDSize();
        };
    }

    /**************************************************************
     * LOAD
     **************************************************************/

    {
        auto &p = params->getParamRef("loadCurrent");
        p.setFn = [this](const QVariant& v){
            return device->setLoadCurrent(v.toInt());
        };
        p.getFn = [this](){
            device->getLoadCurrent();
        };
    }

    /**************************************************************
     * CHARGER
     **************************************************************/

    {
        auto &p = params->getParamRef("chargerCurrent");
        p.setFn = [this](const QVariant& v){
            return device->setChargerCurrent(v.toInt());
        };
        p.getFn = [this](){
            device->getChargerCurrent();
        };
    }

    {
        auto &p = params->getParamRef("chargerTermVoltage");
        p.setFn = [this](const QVariant& v){
            return device->setChargerTermVoltage(v.toFloat());
        };
        p.getFn = [this](){
            device->getChargerTermVoltage();
        };
    }

    /**************************************************************
     * ADC
     **************************************************************/

    {
        auto &p = params->getParamRef("adcResolution");
        p.setFn = [this](const QVariant& v){
            return device->setResolution(static_cast<device_adc_resolution_t>(v.toInt()));
        };
        p.getFn = [this](){
            device->getResolution();
        };
    }

    /**************************************************************
     * CALIBRATION (ReadOnly / handled via batch command)
     **************************************************************/

    {
        auto &p = params->getParamRef("adcVRef");
        p.setFn = nullptr;
        p.getFn = nullptr;
    }

    {
        auto &p = params->getParamRef("adcVOff");
        p.setFn = nullptr;
        p.getFn = nullptr;
    }

    {
        auto &p = params->getParamRef("adcVCor");
        p.setFn = nullptr;
        p.getFn = nullptr;
    }

    {
        auto &p = params->getParamRef("adcVCOffset");
        p.setFn = nullptr;
        p.getFn = nullptr;
    }

    {
        auto &p = params->getParamRef("adcCCor");
        p.setFn = nullptr;
        p.getFn = nullptr;
    }

    /**************************************************************
     * CHARGER
     **************************************************************/
    {
        auto &p = params->getParamRef("chargerHWSerial");
        p.setFn = nullptr;
        p.getFn = [this](){
            device->getChargerHWSerial();
        };
    }

    {
        auto &p = params->getParamRef("chargerFWVersion");
        p.setFn = nullptr;
        p.getFn = [this](){
            device->getChargerFWVersion();
        };
    }
    {
        auto &p = params->getParamRef("chargerChargeCurrent");
        p.setFn = [this](const QVariant& v){
            return device->setChargerCurrent(v.toInt());
        };
        p.getFn = [this](){
            device->getChargerCurrent();
        };
    }

    {
        auto &p = params->getParamRef("chargerTermCurrent");

        p.setFn = [this](const QVariant& v){
            int current = v.toInt();
            int currentId;

            switch(current)
            {
                case 0:
                    currentId = 0;
                    break;

                case 5:
                    currentId = 1;
                    break;

                case 10:
                    currentId = 2;
                    break;

                case 20:
                    currentId = 3;
                    break;

                default:
                    return false;
            }

            return device->setChargerTermCurrent(currentId);
        };

        p.getFn = [this](){
            device->getChargerTermCurrent();
        };
    }

    {
        auto &p = params->getParamRef("chargerMaxChargeCurrent");

        p.setFn = [this](const QVariant& v){
            int current = v.toInt();
            int currentId;

            switch(current)
            {
                case 50:
                    currentId = 0;
                    break;

                case 100:
                    currentId = 1;
                    break;

                case 200:
                    currentId = 2;
                    break;

                case 300:
                    currentId = 3;
                    break;

                case 400:
                    currentId = 4;
                    break;

                case 500:
                    currentId = 5;
                    break;

                case 700:
                    currentId = 6;
                    break;

                case 1100:
                    currentId = 7;
                    break;

                default:
                    return false;
            }

            return device->setChargerMaxChargingCurrent(currentId);
        };

        p.getFn = [this](){
            device->getChargerMaxChargingCurrent();
        };
    }

    {
        auto &p = params->getParamRef("chargerTermVoltage");
        p.setFn = [this](const QVariant& v){
            return device->setChargerTermVoltage(v.toFloat());
        };
        p.getFn = [this](){
            device->getChargerTermVoltage();
        };
    }

    {
        auto &p = params->getParamRef("chargerHWSerial");
        p.setFn = nullptr;
        p.getFn = [this](){
            device->getChargerHWSerial();
        };
    }

    {
        auto &p = params->getParamRef("chargerFWVersion");
        p.setFn = nullptr;
        p.getFn = [this](){
            device->getChargerFWVersion();
        };
    }
}
DeviceContainer::~DeviceContainer()
{
    if(timer)
        timer->stop();

    if(m_dock)
    {
        m_dock->close();
        m_dock->deleteLater();
        m_dock = nullptr;
    }

    if(device)
    {
        device->deleteLater();
        device = nullptr;
    }

    if(deviceWnd)
    {
        deviceWnd->deleteLater();
        deviceWnd = nullptr;
    }

    if(batteryParamsWnd)
    {
        batteryParamsWnd->close();
        batteryParamsWnd->deleteLater();
        batteryParamsWnd = nullptr;
    }
}

bool DeviceContainer::getDeviceName(QString *name)
{
    QString deviceName;
    device->getName(&deviceName);
    *name = deviceName;
    return true;
}

Device *DeviceContainer::getDevice()
{
    return device;
}

DeviceWnd *DeviceContainer::getDeviceWnd()
{
    return deviceWnd;
}

void DeviceContainer::onDeviceControlLinkDisconnected()
{
    log->printLogMessage("Device control link disconnected", LOG_MESSAGE_TYPE_WARNING);
    deviceWnd->setDeviceNetworkState(DEVICE_STATE_DISCONNECTED);
}

void DeviceContainer::onDeviceControlLinkConnected()
{
    log->printLogMessage("Device control link established", LOG_MESSAGE_TYPE_INFO);
    deviceWnd->setDeviceNetworkState(DEVICE_STATE_CONNECTED);
}

void DeviceContainer::onDeviceStatusLinkNewDeviceAdded(QString aDeviceIP)
{
    log->printLogMessage("Status link successfully establish with device(IP: " + aDeviceIP + ")", LOG_MESSAGE_TYPE_INFO);
}

void DeviceContainer::onDeviceStatusLinkNewMessageReceived(QString aDeviceIP, QString aMessage)
{
    if(aMessage.startsWith("dut info", Qt::CaseInsensitive))
    {
        log->printLogMessage(aMessage.mid(QString("dut info").length()).trimmed(), LOG_MESSAGE_TYPE_INFO, LOG_MESSAGE_DEVICE_TYPE_DEVICE, LOG_MESSAGE_CATEGORY_DUT_INFO);
    }
    else
    {
        log->printLogMessage("New message received from device (IP: " + aDeviceIP + ") :\" " + aMessage + "\"", LOG_MESSAGE_TYPE_INFO, LOG_MESSAGE_DEVICE_TYPE_DEVICE);
    }
}

void DeviceContainer::onDeviceWndClosed()
{
    emit sigDeviceClosed(this);
}

void DeviceContainer::onDeviceWndSaveToFileChanged(bool saveToFile)
{
    globalSaveToFileEnabled = saveToFile;
    writeSamplesToFileEnabled = globalSaveToFileEnabled;
    consumptionProfileName = "";
    consumptionProfileNameSet = false;
    consumptionProfileNameExists = false;
}

void DeviceContainer::onDeviceWndEPEnable(bool aEpEnabled)
{
    epEnabled = aEpEnabled;

    bool ok = device->setEPEnable(epEnabled);

    logResult(ok,
              "EP state successfully set",
              "Unable to set EP state");
}

void DeviceContainer::onDeviceWndMaxNumberOfBuffersChanged(unsigned int maxNumber)
{
    bool ok = device->setDataProcessingMaxNumberOfBuffers(maxNumber);

    logResult(ok,
              "Max number of buffers successfully configured",
              "Unable to configure max number of buffers");
}


void DeviceContainer::onDeviceWndConsumptionProfileNameChanged(QString aConsumptionProfileName)
{
    QString fullPath;
    QString deviceName;
    if(consumptionProfileName == aConsumptionProfileName)
    {
        log->printLogMessage("Consumption profile " + consumptionProfileName+ " already exists", LOG_MESSAGE_TYPE_ERROR);
        return;
    }
    if(!device->getName(&deviceName))
    {
        log->printLogMessage("Unable to obtain device name", LOG_MESSAGE_TYPE_ERROR);
    }
    consumptionProfileName = aConsumptionProfileName ;
    consumptionProfileNameSet = true;
    if(!createSubDir(deviceName + "/" + consumptionProfileName, fullPath) )
    {
        log->printLogMessage("Consumption profile " + consumptionProfileName+ " already exists", LOG_MESSAGE_TYPE_WARNING);
        fileProcessing->open(FILEPROCESSING_TYPE_SAMPLES, fullPath);
        fileProcessing->setSamplesFileHeader("Voltage and Current samples");
        fileProcessing->setConsumptionFileHeader("Consumption samples");
        fileProcessing->setSummaryFileHeader("Acquisition info");
        fileProcessing->setEPFileHeader("Energy point info");
        consumptionProfileNameExists = true;
        return;
    }
    if(fileProcessing->open(FILEPROCESSING_TYPE_SAMPLES, fullPath))
    {
        log->printLogMessage("Directory for new consumption profile " + consumptionProfileName + " succesfully created", LOG_MESSAGE_TYPE_INFO);
        fileProcessing->setSamplesFileHeader("Voltage and Current samples");
        fileProcessing->setConsumptionFileHeader("Consumption samples");
        fileProcessing->setSummaryFileHeader("Acquisition info");
        fileProcessing->setEPFileHeader("Energy point info");
        consumptionProfileNameExists = false;
    }
    else
    {
        log->printLogMessage("Unable to open samples log file (Path = " + consumptionProfileName + ")", LOG_MESSAGE_TYPE_ERROR);
    }
}

void DeviceContainer::onDeviceWndLoadStatusChanged(bool status)
{
    bool ok = device->setLoadStatus(status);

    if(ok)
        deviceWnd->setLoadState(status);

    logResult(ok,
              "Load status successfully set",
              "Unable to set load status");
}

void DeviceContainer::onDeviceWndPPathStatusChanged(bool status)
{
    bool ok = device->setPPathStatus(status);

    if(ok)
        deviceWnd->setPPathState(status);

    logResult(ok,
              "PPath status successfully set",
              "Unable to set PPath status");
}

void DeviceContainer::onDeviceWndBatteryStatusChanged(bool status)
{
    bool ok = device->setBatStatus(status);

    if(ok)
        deviceWnd->setBatState(status);

    logResult(ok,
              "Battery status successfully set",
              "Unable to set Battery status");
}

void DeviceContainer::onDeviceWndResetProtection()
{
    bool ok = device->latchTrigger();

    logResult(ok,
              "Protection reset successfully executed",
              "Unable to reset protection");
}

void DeviceContainer::onConsoleWndMessageRcvd(QString msg)
{
    /* call device funtion sendControl Msg -> */
    device->sendControlMsg(msg);
}

void DeviceContainer::onDeviceHandleControlMsgResponse(QString msg, bool exeStatus)
{
    /* call deviceWnd function with recieved msg from FW <- */
    deviceWnd->printConsoleMsg(msg, exeStatus);
}



void DeviceContainer::onDeviceWndSamplesNoChanged(unsigned int newSamplesNo)
{
    bool ok = device->setSamplesNo(newSamplesNo);

    logResult(ok,
              QString::number(newSamplesNo) + " successfully set",
              "Unable to set samples number");
}


void DeviceContainer::onDeviceWndSamplingPeriodChanged(QString time)
{
    bool ok = device->setSamplingPeriod(time);

    /*Everything that maps a marker to a sample works with the sampling period, so it
      is read back from the device after every change*/
    if(ok) device->getSamplingPeriod(NULL);

    logResult(ok,
              "Sampling time successfully set: " + time,
              "Unable to set sampling time");
}

void DeviceContainer::onDeviceWndInterfaceChanged(QString interfaceIp)
{
    int streamID = -1;
    if(!device->createStreamLink(interfaceIp,&streamID))
    {
        log->printLogMessage("Unable to create stream link: ", LOG_MESSAGE_TYPE_ERROR);
        deviceWnd->setDeviceInterfaceSelectionState(DEVICE_INTERFACE_SELECTION_STATE_UNDEFINED);
    }
    else
    {
        log->printLogMessage("Stream link ( sid="+ QString::number(streamID) + " ) successfully created: ", LOG_MESSAGE_TYPE_INFO);
        if(!device->establishEPLink(interfaceIp))
        {
            log->printLogMessage("Unable to create ep link: ", LOG_MESSAGE_TYPE_ERROR);
        }
        else
        {
            log->printLogMessage("Ep link ( port="+ QString::number(8000) + " ) successfully created: ", LOG_MESSAGE_TYPE_INFO);
        }
        deviceWnd->setDeviceInterfaceSelectionState(DEVICE_INTERFACE_SELECTION_STATE_SELECTED);
        device->acquireDeviceConfiguration(DEVICE_ADC_EXTERNAL);
        if(!device->establishStatusLink(interfaceIp))
        {
            log->printLogMessage("Unable to create status link: ", LOG_MESSAGE_TYPE_ERROR);
        }
        else
        {
            log->printLogMessage("Status link ( port="+ device->parameters()->getParamValue("statusLinkPort") + " ) successfully created: ", LOG_MESSAGE_TYPE_INFO);
        }
    }
}


void DeviceContainer::onDeviceWndAcquisitionStart()
{
    if(globalSaveToFileEnabled && (!consumptionProfileNameSet))
    {
        QMessageBox msgBox;
        msgBox.setWindowIcon(QIcon(QPixmap(":/images/NewSet/stopHand.png")));
        msgBox.setWindowTitle("Error: Unable to start Acquisition");
        msgBox.setText("Set consumption profile name or disable \"Save to file\"");
        msgBox.exec();
        return;
    }
    if(globalSaveToFileEnabled && consumptionProfileNameExists)
    {
        QMessageBox msgBox;
        msgBox.setWindowIcon(QIcon(QPixmap(":/images/NewSet/stopHand.png")));
        msgBox.setWindowTitle("Warning: Consumption profile already exists");
        msgBox.setInformativeText("Consumption profile already exists. Continuing will overwrite previous data. Do you want to proceed?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        int ret = msgBox.exec();

        switch (ret) {
        case QMessageBox::Ok:
            fileProcessing->reOpenFiles();
            consumptionProfileNameExists = false;
            break;
        case QMessageBox::Cancel:
            return;
        default:
            return;
        }
    }
    if(!device->acquisitionStart())
    {
        log->printLogMessage("Unable to start acquistion", LOG_MESSAGE_TYPE_ERROR);
    }
    else
    {
        log->printLogMessage("Acquisition successfully started", LOG_MESSAGE_TYPE_INFO);
        deviceWnd->setDeviceAcqState(DEVICE_ACQ_ACTIVE);
        if(globalSaveToFileEnabled)
        {
            fileProcessing->appendSummaryFile("EP Enabled: " + QString::number(epEnabled));
            fileProcessing->appendSummaryFile("Acquisiton start: " + QDateTime::currentDateTime().toString());
        }
    }
}

void DeviceContainer::onDeviceWndAcquisitionStop()
{
    if(!device->acquisitionStop())
    {
        log->printLogMessage("Unable to stop acquistion", LOG_MESSAGE_TYPE_ERROR);
    }
    else
    {
        log->printLogMessage("Acquisition successfully stoped", LOG_MESSAGE_TYPE_INFO);
        if(globalSaveToFileEnabled && (consumptionProfileNameExists == false))
        {
            fileProcessing->appendSummaryFile("Acquisiton stop: " + QDateTime::currentDateTime().toString());
            consumptionProfileNameExists = true;
        }
        deviceWnd->setDeviceAcqState(DEVICE_ACQ_PAUSE);
    }
}

void DeviceContainer::onDeviceWndAcquisitionPause()
{
    if(!device->acquisitionPause())
    {
        log->printLogMessage("Unable to pause acquistion", LOG_MESSAGE_TYPE_ERROR);
    }
    else
    {
        log->printLogMessage("Acquisition successfully paused", LOG_MESSAGE_TYPE_INFO);
          if(globalSaveToFileEnabled && (consumptionProfileNameExists == false))
        {
            fileProcessing->appendSummaryFile("Acquisiton stop: " + QDateTime::currentDateTime().toString());
            consumptionProfileNameExists = true;
        }
        deviceWnd->setDeviceAcqState(DEVICE_ACQ_PAUSE);
    }
}

void DeviceContainer::onDeviceWndAcquisitionRefresh()
{
    device->acquireDeviceConfiguration();
}



void DeviceContainer::onDeviceSamplingPeriodObtained(QString stime)
{
    bool ok = deviceWnd->setSamplingPeriod(stime);

    /*Device reports the sampling period in microseconds*/
    samplingPeriodMs = stime.toDouble() / 1000.0;
    batteryParamsExtraction->setSamplingPeriod(samplingPeriodMs);

    log->printLogMessage("Battery parameters: sampling period " + QString::number(samplingPeriodMs, 'f', 4) + " ms",
                         LOG_MESSAGE_TYPE_INFO);

    logResult(ok,
              "Sampling time successfully obtained and presented",
              "Unable to present sampling time");
}
void DeviceContainer::onDeviceConfigUpdate(QMap<QString, QString> changedFields)
{
    auto params = device->parameters();
    bool status = true;
    for(auto it = changedFields.begin(); it != changedFields.end(); ++it)
    {
        const QString &key = it.key();
        const QString &value = it.value();

        if(!params->hasParam(key))
        {
            log->printLogMessage("Unknown param: " + key, LOG_MESSAGE_TYPE_WARNING);
            continue;
        }

        auto &param = params->getParamRef(key);

        if(param.setFn)
        {
            param.setFn(value);
        }
        else
        {
            log->printLogMessage("No setFn for param: " + key, LOG_MESSAGE_TYPE_WARNING);
            status = false;
        }
    }
    deviceWnd->setConfigurationAppliedStatus(status);
}

void DeviceContainer::onDeviceConfigGet()
{
    device->acquireDeviceConfiguration();
    deviceWnd->setConfigurationAppliedStatus(true);
}

void DeviceContainer::onDeviceConfigStore()
{
    bool ok = device->storeParam();

    logResult(ok,
              "Parameter stored to FS",
              "Unable to store parameters");

}

void DeviceContainer::onDeviceReset()
{
    bool ok = device->reset();

    log->printLogMessage("Reset device ...", LOG_MESSAGE_TYPE_WARNING);

    if(timer)
        timer->stop();

    emit sigDeviceClosed(this);   // 🔥 SAMO OVO

    return;
}

void DeviceContainer::onDeviceBDChunkRead(float percentage)
{
    deviceWnd->setConfigurationBDProgressStatus(percentage, "Reading BD content ...");
}

void DeviceContainer::onDeviceBDChunkWrite(float percentage)
{
    deviceWnd->setConfigurationBDProgressStatus(percentage, "Writting BD content ...");
}
void DeviceContainer::onDeviceSamplingTimeChanged(double value)
{
    deviceWnd->setStatisticsSamplingTime(value);
}



void DeviceContainer::onDeviceWndLoadCurrentSetStatus(bool status)
{
    QString statusStr = status ? "Enabled" : "Disabled";

    bool ok = device->setDACStatus(status);

    if(ok)
        deviceWnd->setLoadCurrentStatus(status);

    logResult(ok,
              "DAC status successfully set: " + statusStr,
              "Unable to set DAC status");
}

void DeviceContainer::onDeviceWndChargingCurrentSetValue(unsigned int current)
{
    bool ok = device->setChargerCurrent(current);

    logResult(ok,
              "Charging current successfully set: " + QString::number(current) + " [mA]",
              "Unable to set Charging current");
}

void DeviceContainer::onDeviceWndChargingTermCurrentSetValue(unsigned int current)
{
    bool ok = device->setChargerTermCurrent(current);

    logResult(ok,
              "Charging termination current successfully set: " + QString::number(current) + " [%]",
              "Unable to set Charging termination current");
}

void DeviceContainer::onDeviceWndChargingTermVoltageSetValue(float voltage)
{
    bool ok = device->setChargerTermVoltage(voltage);

    logResult(ok,
              "Charging termination voltage successfully set: " + QString::number(voltage, 'g', 3) + " [V]",
              "Unable to set Charging termination voltage");
}

void DeviceContainer::onDeviceWndChargingCurrentSetStatus(bool status)
{
    QString statusStr = status ? "Enabled" : "Disabled";

    bool ok = device->setChargerStatus(status);

    if(ok)
        deviceWnd->setChargingCurrentStatus(status);

    logResult(ok,
              "Charger status successfully set: " + statusStr,
              "Unable to set Charger status");
}

void DeviceContainer::onDeviceWndChDschSaveToFileChanged(bool saveToFile)
{

    if(!globalSaveToFileEnabled)
    {
        log->printLogMessage("Global save to file option is not enabled", LOG_MESSAGE_TYPE_ERROR);
        return;
    }
    if(!consumptionProfileNameSet)
    {
        log->printLogMessage("Unable to enable write to file. Please set profile name ", LOG_MESSAGE_TYPE_ERROR);
        return;
    }
    chDschSaveToFileEnabled = saveToFile;
    writeSamplesToFileEnabled = globalSaveToFileEnabled && chDschSaveToFileEnabled;

    log->printLogMessage("Write samples to file set to: " + QString(writeSamplesToFileEnabled ? "Enabled": "Disabled"), LOG_MESSAGE_TYPE_INFO);

}

void DeviceContainer::onDeviceWndGetBDContent()
{
    QString content;
    bool exeStatus = device->getBDFContentFull(&content);
    logResult(exeStatus,
              "Full external memory block device content read",
              "Unable to read full  memory block device content");

    if(exeStatus)
    {
        deviceWnd->setBDContent(content);
        deviceWnd->setConfigurationBDProgressStatus(100.0, "BD Read Successfully");
    }
}

void DeviceContainer::onDeviceWndSetBDContent(QByteArray content)
{
    bool exeStatus = device->setBDFContent(&content);
    logResult(exeStatus,
              "Block device content set",
              "Unable to set memory block content");
    if(exeStatus)
    {
        deviceWnd->setConfigurationBDProgressStatus(100.0, "BD Written Successfully");
    }
}

void DeviceContainer::onDeviceWndBDFormat()
{
    bool exeStatus = device->BDFormat();
    logResult(exeStatus,
               "Block device formated sucesfully",
               "Unable to format block device");

}

void DeviceContainer::onDeviceWndChargerGetBDContent()
{

    QString content;
    bool exeStatus = device->getChargerBDContentFull(&content);
    logResult(exeStatus,
              "Full charger memory content read",
              "Unable to read charger memory ");

    if(exeStatus)
    {
        deviceWnd->setChargerBDContent(content);
        deviceWnd->setConfigurationChargerBDProgressStatus(100.0, "BD Read Successfully");
    }
}

void DeviceContainer::onDeviceWndChargerSetBDContent(QByteArray content)
{
    bool exeStatus = device->setChargerBDContent(&content);
    logResult(exeStatus,
              "Block device content set",
              "Unable to set memory block content");
    if(exeStatus)
    {
        deviceWnd->setConfigurationBDProgressStatus(100.0, "BD Written Successfully");
    }
}

void DeviceContainer::onDeviceWndChargerBDFormat()
{

}

void DeviceContainer::onDeviceAcquisitonStarted()
{
    elapsedTime = 0;
    timer->start(1000);
    batteryParamsExtraction->onReset();

}

void DeviceContainer::onDeviceAcquisitonStopped()
{
    elapsedTime = 0;
    timer->stop();
    deviceWnd->setStatisticsElapsedTime(elapsedTime);
}

void DeviceContainer::onTimeout()
{
    elapsedTime += 1;
    deviceWnd->setStatisticsElapsedTime(elapsedTime);
}

void DeviceContainer::onDeviceNewVoltageCurrentSamplesReceived(QVector<double> voltage, QVector<double> current, QVector<double> voltageKeys, QVector<double> currentKeys)
{
    deviceWnd->plotVoltageValues(voltage, voltageKeys);
    deviceWnd->plotCurrentValues(current, currentKeys);
    batteryParamsExtraction->onNewSamplesReceived(voltage, current, voltageKeys, currentKeys);

    /*Averaged voltage belongs to a running battery parameters wave, outside of it
      there is nothing to relax and the trace would only clutter the plot*/
    if(batParamRelaxRunning && (batParamRelaxFilter > 0))
    {
        QVector<double> averaged;

        batParamAverageProcess(voltage, voltageKeys, &averaged);
        deviceWnd->plotVoltageAverageValues(averaged, voltageKeys);
        batParamRelaxationSamplesProcess(averaged, voltageKeys);
    }
    else
    {
        batParamRelaxationSamplesProcess(voltage, voltageKeys);
    }
    if(writeSamplesToFileEnabled)
    {
        fileProcessing->appendSampleDataQueued(voltage, voltageKeys, current, currentKeys);
    }
}

void DeviceContainer::onDeviceNewConsumptionDataReceived(QVector<double> consumption, QVector<double> keys, dataprocessing_consumption_mode_t mode)
{
    deviceWnd->plotConsumptionValues(consumption, keys);
    if(writeSamplesToFileEnabled)
    {
        fileProcessing->appendConsumptionQueued(consumption, keys);
    }
}

void DeviceContainer::onDeviceNewStatisticsReceived(dataprocessing_dev_info_t voltageStat, dataprocessing_dev_info_t currentStat, dataprocessing_dev_info_t consumptionStat)
{
    device_stat_info statInfo;
    statInfo.voltageAvg = voltageStat.average;
    statInfo.voltageMax = voltageStat.max;
    statInfo.voltageMin = voltageStat.min;
    statInfo.currentAvg = currentStat.average;
    statInfo.currentMax = currentStat.max;
    statInfo.currentMin = currentStat.min;
    statInfo.consumptionAvg = consumptionStat.average;
    statInfo.consumptionMax = consumptionStat.max;
    statInfo.consumptionMin = consumptionStat.min;
    deviceWnd->showStatistic(statInfo);
}

void DeviceContainer::onDeviceNewSamplesBuffersProcessingStatistics(double dropRate, unsigned int dropPacketsNo, unsigned int fullReceivedBuffersNo, unsigned int lastBufferID, unsigned short ebp)
{
    deviceWnd->setStatisticsData(dropRate, dropPacketsNo, fullReceivedBuffersNo, lastBufferID);
}

void DeviceContainer::onDeviceNewEBP(QVector<double> ebpValues, QVector<double> keys)
{
    //deviceWnd->plotAppendConsumptionEBP(ebpValues, keys);
}

void DeviceContainer::onDeviceNewEBPFull(double value, double key, QString name)
{
    deviceWnd->plotConsumptionEBPWithName(value, key, name);
    batteryParamsExtraction->onNewMarkerReceived(value, key, name);
    if(writeSamplesToFileEnabled)
    {
        fileProcessing->appendEPQueued(name, key);
    }
    log->printLogMessage(name + " (value: " + QString::number(value) + ", key: " + QString::number(key) + ")", LOG_MESSAGE_TYPE_INFO, LOG_MESSAGE_DEVICE_TYPE_DEVICE, LOG_MESSAGE_CATEGORY_ENERGY_POINT);

    checkAcquisitionPauseMarker(name);
    batParamRelaxationMarkerProcess(name, key);
}

void DeviceContainer::onDeviceWndBatParamRelaxationChanged(bool enabled, double thresholdMv, double windowS, double filterMs)
{
    batParamRelaxEnabled = enabled;
    batParamRelaxThreshold = thresholdMv / 1000.0;
    batParamRelaxWindow = windowS * 1000.0;
    batParamRelaxFilter = filterMs;

    batParamRelaxationReset();
    deviceWnd->enableVoltageAveragePlot(false);

    /*Load tab drives the procedure, so the analysis works with the same relaxation
      parameters the pause was cut with*/
    batteryparams_settings_t settings = batteryParamsExtraction->getSettings();

    settings.relaxationThreshold = thresholdMv;
    settings.relaxationWindow = windowS;
    settings.relaxationFilter = filterMs;

    batteryParamsExtraction->setSettings(settings);

    if(batteryParamsWnd != NULL) batteryParamsWnd->setSettings(settings);

    if(!enabled) return;

    log->printLogMessage("Battery parameters: relaxation ends on " + QString::number(thresholdMv) +
                          " mV in " + QString::number(windowS) + " s, voltage averaged over " + QString::number(filterMs) +
                         " ms, runs until under voltage protection",
                         LOG_MESSAGE_TYPE_INFO);
}

void DeviceContainer::batParamAverageProcess(QVector<double> voltage, QVector<double> voltageKeys, QVector<double> *averaged)
{
    if(averaged != NULL) averaged->clear();

    for(int i = 0; i < voltage.size() && i < voltageKeys.size(); i++)
    {
        batParamAverageKeys.append(voltageKeys[i]);
        batParamAverageVoltage.append(voltage[i]);
        batParamAverageSum += voltage[i];

        while((batParamAverageFirst < (batParamAverageKeys.size() - 1)) &&
              ((batParamAverageKeys.last() - batParamAverageKeys[batParamAverageFirst]) > batParamRelaxFilter))
        {
            batParamAverageSum -= batParamAverageVoltage[batParamAverageFirst];
            batParamAverageFirst++;
        }

        if(averaged != NULL)
        {
            averaged->append(batParamAverageSum / (double)(batParamAverageKeys.size() - batParamAverageFirst));
        }
    }

    /*Only the samples inside the filter window matter, the rest is dropped so that a
      long measurement does not keep growing the buffer*/
    if(batParamAverageFirst > 0)
    {
        batParamAverageKeys.remove(0, batParamAverageFirst);
        batParamAverageVoltage.remove(0, batParamAverageFirst);
        batParamAverageFirst = 0;
    }
}

void DeviceContainer::onBatParamCycleFinished(batteryparams_cycle_t cycle)
{
    log->printLogMessage("Battery parameters: cycle " + QString::number(cycle.index) + " extracted, R = " +
                         (cycle.resistanceValid ? QString::number(cycle.resistance * 1000.0, 'f', 1) + " mOhm" : QString("n/a")) +
                         ", relaxed " + (cycle.relaxationReached ? QString("yes") : QString("no")),
                         LOG_MESSAGE_TYPE_INFO);
}

void DeviceContainer::batParamRelaxationReset()
{
    batParamRelaxCheckKey = 0;
    batParamAverageSum = 0;
    batParamAverageFirst = 0;
    batParamAverageKeys.clear();
    batParamAverageVoltage.clear();
    batParamRelaxPassesDone = 0;
    batParamRelaxPauseActive = false;
    batParamRelaxRunning = false;
    batParamRelaxRestarting = false;
    batParamRelaxKeys.clear();
    batParamRelaxVoltage.clear();
}

void DeviceContainer::batParamRelaxationMarkerProcess(QString name, double key)
{
    Q_UNUSED(key);

    if(!batParamRelaxEnabled || !batParamRelaxRunning) return;

    if(name.trimmed().compare(BATTERYPARAMS_DEFAULT_PAUSE_START_MARKER, Qt::CaseInsensitive) == 0)
    {
        batParamRelaxPauseActive = true;
        batParamRelaxKeys.clear();
        batParamRelaxVoltage.clear();
        batParamRelaxCheckKey = 0;
        return;
    }

    if(name.trimmed().compare(BATTERYPARAMS_DEFAULT_PAUSE_END_MARKER, Qt::CaseInsensitive) == 0)
    {
        batParamRelaxPauseActive = false;
        batParamRelaxKeys.clear();
        batParamRelaxVoltage.clear();
        batParamRelaxCheckKey = 0;
        return;
    }
}

void DeviceContainer::batParamRelaxationSamplesProcess(QVector<double> voltage, QVector<double> voltageKeys)
{
    int relaxPosition;

    if(!batParamRelaxEnabled || !batParamRelaxRunning || !batParamRelaxPauseActive) return;
    if(batParamRelaxThreshold <= 0 || batParamRelaxWindow <= 0) return;

    for(int i = 0; i < voltage.size() && i < voltageKeys.size(); i++)
    {
        batParamRelaxKeys.append(voltageKeys[i]);
        batParamRelaxVoltage.append(voltage[i]);
    }

    if(batParamRelaxKeys.size() < 2) return;

    /*Whole pause is kept, the relaxation point can lie far behind the newest sample.
      A long pause is still bounded so that a stuck measurement cannot eat the memory*/
    if(batParamRelaxKeys.size() > DEVICECONTAINER_RELAX_SAMPLES_MAX)
    {
        int excess = batParamRelaxKeys.size() - DEVICECONTAINER_RELAX_SAMPLES_MAX;
        batParamRelaxKeys.remove(0, excess);
        batParamRelaxVoltage.remove(0, excess);
    }

    /*Evaluating the whole pause on every packet would be wasted work, the pause is
      minutes long and the search is done over a growing array*/
    if((batParamRelaxKeys.last() - batParamRelaxCheckKey) < DEVICECONTAINER_RELAX_CHECK_PERIOD) return;

    batParamRelaxCheckKey = batParamRelaxKeys.last();

    /*Same search the analysis uses, so the relaxation point reported in the battery
      parameters window is the one the pause was ended on*/
    relaxPosition = BatteryParamsExtraction::relaxationPositionGet(batParamRelaxKeys, batParamRelaxVoltage,
                                                                   0, batParamRelaxKeys.size() - 1,
                                                                   batParamRelaxThreshold, batParamRelaxWindow);

    if(relaxPosition < 0) return;

    log->printLogMessage("Battery parameters: relaxed at " + QString::number(batParamRelaxKeys[relaxPosition] / 1000.0, 'f', 3) + " s",
                         LOG_MESSAGE_TYPE_INFO);

    batParamRelaxationFinish(batParamRelaxKeys.last());
}

void DeviceContainer::batParamRelaxationFinish(double key)
{
    QString marker = BATTERYPARAMS_DEFAULT_PAUSE_END_MARKER;
    double step = samplingPeriodMs;
    double index;

    /*Markers are addressed by the sample index while the collected keys are times,
      so the step between two samples is needed to place the marker*/
    if((step <= 0) && (batParamRelaxKeys.size() > 1))
    {
        step = batParamRelaxKeys[batParamRelaxKeys.size() - 1] - batParamRelaxKeys[batParamRelaxKeys.size() - 2];
    }

    if(step <= 0)
    {
        log->printLogMessage("Battery parameters: unknown sampling period, unable to place pause end marker", LOG_MESSAGE_TYPE_ERROR);
        index = 0;
    }
    else
    {
        index = qRound(key / step);
    }

    batParamRelaxPauseActive = false;
    batParamRelaxKeys.clear();
    batParamRelaxVoltage.clear();
    batParamRelaxCheckKey = 0;

    /*The device is stopped in the middle of the pause chunk, so it never sends the
      pause end marker. The marker is created here instead, at the moment the voltage
      settled, so that the extraction and the stored energy point file stay complete*/
    deviceWnd->plotConsumptionEBPWithName(0, index, marker);
    batteryParamsExtraction->onNewMarkerReceived(0, index, marker);
    if(writeSamplesToFileEnabled) fileProcessing->appendEPQueued(marker, (int)index);

    log->printLogMessage("Battery parameters: pass " + QString::number(batParamRelaxPassesDone + 1) + " relaxed, pause ended early",
                         LOG_MESSAGE_TYPE_INFO, LOG_MESSAGE_DEVICE_TYPE_DEVICE, LOG_MESSAGE_CATEGORY_ENERGY_POINT);

    batParamRelaxRestarting = true;
    if(!device->setLoadWaveState(false))
    {
        log->printLogMessage("Unable to stop load wave", LOG_MESSAGE_TYPE_ERROR);
        batParamRelaxRestarting = false;
        return;
    }

    batParamRelaxationNextPass();
}

void DeviceContainer::batParamRelaxationNextPass()
{
    batParamRelaxRestarting = false;
    batParamRelaxPassesDone++;

    /*Procedure has no pass count, it runs until the battery hits the under voltage
      protection and the protection stops the wave*/
    if(!device->setLoadWaveState(true))
    {
        batParamRelaxRunning = false;
        deviceWnd->enableVoltageAveragePlot(false);
        deviceWnd->loadWaveStopped();
        log->printLogMessage("Unable to start next battery parameters pass", LOG_MESSAGE_TYPE_ERROR);
    }
}

void DeviceContainer::checkAcquisitionPauseMarker(QString name)
{
    auto params = device->parameters();
    if(params == NULL) return;
    if(params->getParamVariant("acqStopOnMarkerEnabled").toBool() == false) return;

    QString pauseMarker = params->getParamValue("acqStopOnMarkerName").trimmed();
    if(pauseMarker.isEmpty()) return;
    if(name.trimmed().compare(pauseMarker, Qt::CaseInsensitive) != 0) return;

    log->printLogMessage("Pause marker \"" + pauseMarker + "\" received, pausing acquisition", LOG_MESSAGE_TYPE_INFO);

    onDeviceWndAcquisitionPause();

    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(QPixmap(":/images/NewSet/pause.png")));
    msgBox.setWindowTitle("Acquisition paused");
    msgBox.setText("Acquisition paused because marker \"" + pauseMarker + "\" was detected.");
    msgBox.exec();
}

void DeviceContainer::onDeviceMeasurementEnergyFlowStatusChanged(charginganalysis_status_t status)
{
    QString msg = "Charging status changed to ";
    QString wndMsg;
    switch(status)
    {
    case CHARGINGANALYSIS_STATUS_UNKNOWN:
        msg += "Uknown";
        wndMsg = "Uknown";
        break;
    case CHARGINGANALYSIS_STATUS_IDLE:
        msg += "Idle";
        wndMsg = "Idle";
        break;
    case CHARGINGANALYSIS_STATUS_CHARGING:
        msg += "Charging";
        wndMsg = "Charging";
        break;
    case CHARGINGANALYSIS_STATUS_DISCHARGING:
        msg += "Discharging";
        wndMsg = "Discharging";
        break;

    }
    deviceWnd->setChargingStatus(wndMsg);
    log->printLogMessage(msg, LOG_MESSAGE_TYPE_INFO);
}

void DeviceContainer::onDeviceWndCalibrationUpdated()
{
    bool ok;

    device->calibrationUpdated();

    /*Voltage and current calibration parameters are used by the application itself,
      but the load DAC offset is applied on the device, so every update has to be
      sent there as well*/
    ok = device->setCalParam();

    logResult(ok,
              "Calibration parameters successfully applied on device",
              "Unable to apply calibration parameters on device");
}

void DeviceContainer::onDeviceWndAutoCalApply()
{
    /*Recompute the increments so the live measurement reflects the parameter the
      wizard just changed*/
    device->calibrationUpdated();
}

void DeviceContainer::onDeviceWndAutoCalSetLoadCurrent(int mA)
{
    device->setLoadCurrent(mA);
}

void DeviceContainer::onDeviceWndAutoCalSetLoadEnabled(bool enabled)
{
    bool ok;

    if(enabled)
    {
        /*A protection can latch while the source is being connected, so it is reset
          before the load is enabled instead of refusing the calibration*/
        if(uVoltageActive || oVoltageActive || oCurrentActive)
        {
            device->latchTrigger();
            uVoltageActive = false;
            oVoltageActive = false;
            oCurrentActive = false;
            deviceWnd->autoCalibrationLog("Protection was active, latch reset");
        }
    }

    /*Load output is driven directly here, not through the energy control window, so
      the calibration does not depend on the energy control working mode. The load
      stage is off by default, so it is enabled together with the DAC*/
    if(enabled)
    {
        bool loadOk = device->setLoadStatus(true);
        bool dacOk = device->setDACStatus(true);

        ok = loadOk && dacOk;
        if(ok) deviceWnd->autoCalibrationLog("Load and DAC enabled");
        deviceWnd->autoCalibrationLoadEnableResult(ok);
    }
    else
    {
        device->setDACStatus(false);
        device->setLoadStatus(false);
        ok = true;
    }
}

void DeviceContainer::onDeviceWndAutoCalResetProtection()
{
    device->latchTrigger();
    uVoltageActive = false;
    oVoltageActive = false;
    oCurrentActive = false;
    deviceWnd->autoCalibrationLog("Protection latch reset requested");
}

void DeviceContainer::onDeviceWndCalibrationStoreRequest()
{
    bool ok = device->setCalParam();
    bool ok2 = device->storeParam();
    logResult(ok && ok2,
              "Calibration parameters sucesfully updated and stored on device",
              "Unable to update calibration parameters");
}


void DeviceContainer::onDeviceLoadStateObtained(bool state)
{
    bool ok = deviceWnd->setLoadState(state);

    logResult(ok,
              "Load state successfully obtained and presented",
              "Unable to obtain load state");
}

void DeviceContainer::onDeviceBatStateObtained(bool state)
{
    bool ok = deviceWnd->setBatState(state);

    logResult(ok,
              "Battery state successfully obtained and presented",
              "Unable to obtain battery state");
}

void DeviceContainer::onDevicePPathStateObtained(bool state)
{
    bool ok = deviceWnd->setPPathState(state);

    logResult(ok,
              "PPath state successfully obtained and presented",
              "Unable to obtain PPath state");
}

void DeviceContainer::onDeviceChargerStateObtained(bool state)
{
    bool ok = deviceWnd->setChargerState(state);

    logResult(ok,
              "Charger state successfully obtained and presented",
              "Unable to obtain charger state");
}

void DeviceContainer::onDeviceDACStateObtained(bool state)
{
    bool ok = deviceWnd->setDACState(state);

    logResult(ok,
              "DAC state successfully obtained and presented",
              "Unable to obtain DAC state");
}

void DeviceContainer::onDeviceLoadCurrentObtained(int current)
{
    bool ok = deviceWnd->setLoadCurrent(current);

    logResult(ok,
              "Load current successfully obtained and presented",
              "Unable to obtain load current");
}

void DeviceContainer::onDeviceUVoltageObtained(bool state)
{
    uVoltageActive = state;
    if(state) deviceWnd->autoCalibrationLog("Under voltage protection tripped");
    bool ok = deviceWnd->setUVoltageIndication(state);

    logResult(ok,
              "Under Voltage protection state successfully obtained and presented",
              "Unable to obtain Under Voltage protection state");
}

void DeviceContainer::onDeviceChargerConnectionStatusOntained(bool state)
{
    deviceWnd->setChargerConnectionStatus(state);

    if(state == true)
    {
        QString hwSerial, fwversion;
        device->getChargerCurrent();
        device->getChargerTermCurrent();
        device->getChargerTermVoltage();
        device->getChargerFWVersion(&fwversion);
        device->getChargerHWSerial(&hwSerial);
        device->getChargerMaxChargingCurrent();
        log->printLogMessage("Charger connected", LOG_MESSAGE_TYPE_INFO);
    }
    else
    {
        log->printLogMessage("Charger disconnected", LOG_MESSAGE_TYPE_WARNING);
    }

}

void DeviceContainer::onDeviceOVoltageObtained(bool state)
{
    oVoltageActive = state;
    if(state) deviceWnd->autoCalibrationLog("Over voltage protection tripped");
    bool ok = deviceWnd->setOVoltageIndication(state);

    logResult(ok,
              "Over Voltage protection state successfully obtained and presented",
              "Unable to obtain Over Voltage protection state");
}

void DeviceContainer::onDeviceOCurrentObtained(bool state)
{
    oCurrentActive = state;
    if(state) deviceWnd->autoCalibrationLog("Over current protection tripped");
    bool ok = deviceWnd->setOCurrentIndication(state);

    logResult(ok,
              "Over Current protection state successfully obtained and presented",
              "Unable to obtain Over Current protection state");
}

void DeviceContainer::onDeviceUVoltageValueObtained(float value)
{
    bool ok = deviceWnd->setUVoltageValue(value);

    logResult(ok,
              "Under Voltage value successfully obtained and presented",
              "Unable to obtain Under Voltage value");
}

void DeviceContainer::onDeviceOVoltageValueObtained(float value)
{
    bool ok = deviceWnd->setOVoltageValue(value);

    logResult(ok,
              "Over Voltage value successfully obtained and presented",
              "Unable to obtain Over Voltage value");
}

void DeviceContainer::onDeviceOCurrentValueObtained(int value)
{
    bool ok = deviceWnd->setOCurrentValue(value);

    logResult(ok,
              "Over Current value successfully obtained and presented",
              "Unable to obtain Over Current value");
}

void DeviceContainer::onDeviceBDSizeObtained(int value)
{
    bool ok = deviceWnd->setBDSize(value);

    logResult(ok,
              "Block Device Size successfully obtained and presented",
              "Unable to obtain Block Device Size");
}

void DeviceContainer::onDeviceChargingDone()
{
    bool ok = deviceWnd->chargingDone();

    logResult(ok,
              "Charging completed successfully",
              "Unable to process charging done event");
}

void DeviceContainer::onDeviceChargerCurrentObtained(int current)
{
    bool ok = deviceWnd->setChargerCurrent(current);

    logResult(ok,
              "Charge current successfully obtained and presented",
              "Unable to obtain charge current");
}

void DeviceContainer::onDeviceChargerTermCurrentObtained(int current)
{
    bool ok = deviceWnd->setChargerTermCurrent(current);

    logResult(ok,
              "Charge termination current successfully obtained and presented",
              "Unable to obtain charge termination current");
}

void DeviceContainer::onDeviceChargerTermVoltageObtained(float voltage)
{
    bool ok = deviceWnd->setChargerTermVoltage(voltage);

    logResult(ok,
              "Charge termination voltage successfully obtained and presented",
              "Unable to obtain charge termination voltage");
}
void DeviceContainer::onDeviceChargerMaxCurrentObtained(int current)
{
    bool ok = deviceWnd->setChargerMaxCurrent(current);

    logResult(ok,
              "Charger max current successfully obtained and presented",
              "Unable to obtain charger max current");
}
void DeviceContainer::onDeviceChargerHWSerialObtained(QString serial)
{
    bool ok = deviceWnd->setChargerHWSerial(serial);

    logResult(ok,
              "Charger serial number obtained and presented",
              "Unable to obtain charger serial number");
}

void DeviceContainer::onDeviceChargerFWVersionObtained(QString serial)
{
    bool ok = deviceWnd->setChargerFWSerial(serial);

    logResult(ok,
              "Charger fw version successfully obtained and presented",
              "Unable to obtain charger fw version");
}

void DeviceContainer::onDeviceWndLoadWaveSet(Waveform wave)
{
    bool ok = device->setLoadWave(wave);

    logResult(ok,
              "Load wave \"" + wave.name + "\" successfully set (" + QString::number(wave.chunks.size()) + " chunks)",
              "Unable to set load wave \"" + wave.name + "\"");
}

void DeviceContainer::onBatParamSettingsChanged(batteryparams_settings_t settings)
{
    batteryParamsExtraction->setSettings(settings);
}

void DeviceContainer::onDeviceWndBatParamCapacityChanged(double capacity)
{
    batteryparams_settings_t settings = batteryParamsExtraction->getSettings();

    settings.capacity = capacity;
    batteryParamsExtraction->setSettings(settings);

    if(batteryParamsWnd != NULL) batteryParamsWnd->setSettings(settings);
}

void DeviceContainer::onDeviceWndBatParamViewRequested()
{
    if(batteryParamsWnd == NULL)
    {
        batteryParamsWnd = new BatteryParamsWnd();
        batteryParamsWnd->setAttribute(Qt::WA_QuitOnClose, false);
        batteryParamsWnd->setProfileName(consumptionProfileName);
        batteryParamsWnd->setSettings(batteryParamsExtraction->getSettings());

        connect(batteryParamsExtraction, &BatteryParamsExtraction::sigCycleStarted,
                batteryParamsWnd, &BatteryParamsWnd::onCycleStarted, Qt::QueuedConnection);
        connect(batteryParamsExtraction, &BatteryParamsExtraction::sigCycleFinished,
                batteryParamsWnd, &BatteryParamsWnd::onCycleFinished, Qt::QueuedConnection);
        connect(batteryParamsWnd, &BatteryParamsWnd::sigSettingsChanged,
                this, &DeviceContainer::onBatParamSettingsChanged);

        /*Window is usually opened while the procedure already runs, so the cycles
          extracted before it existed are handed over*/
        QVector<batteryparams_cycle_t> collected = batteryParamsExtraction->getCycles();

        for(int i = 0; i < collected.size(); i++)
        {
            batteryParamsWnd->onCycleFinished(collected[i]);
        }
    }

    batteryParamsWnd->show();
    batteryParamsWnd->raise();
    batteryParamsWnd->activateWindow();
}

void DeviceContainer::onDeviceWndLoadWaveSetStatus(bool status)
{
    QString statusStr = status ? "Started" : "Stopped";

    bool ok = device->setLoadWaveState(status);

    if(ok)
    {
        batParamRelaxationReset();
        batParamRelaxRunning = (status && batParamRelaxEnabled);
        deviceWnd->enableVoltageAveragePlot(batParamRelaxRunning && (batParamRelaxFilter > 0));
    }

    if(ok && status)
    {
        batteryParamsExtraction->onReset();
        if(batteryParamsWnd != NULL)
        {
            batteryParamsWnd->setProfileName(consumptionProfileName);
            batteryParamsWnd->onClear();
        }
    }

    if(ok)
        deviceWnd->setLoadCurrentStatus(status);

    logResult(ok,
              "Load wave " + statusStr,
              "Unable to " + QString(status ? "start" : "stop") + " load wave");
}

void DeviceContainer::onDeviceLoadWaveStopped()
{
    bool ok;

    if(batParamRelaxRestarting) return;

    if(batParamRelaxEnabled && batParamRelaxRunning)
    {
        log->printLogMessage("Battery parameters: pass " + QString::number(batParamRelaxPassesDone + 1) + " finished on maximum pause",
                             LOG_MESSAGE_TYPE_INFO);
        batParamRelaxPauseActive = false;
        batParamRelaxationNextPass();
        return;
    }

    ok = deviceWnd->loadWaveStopped();

    logResult(ok,
              "Load wave completed",
              "Unable to process load wave stopped event");
}

void DeviceContainer::onDeviceWndLoadWaveClear()
{
    bool ok = device->clearLoadWave();

    logResult(ok,
              "Load wave cleared",
              "Unable to clear load wave");
}

void DeviceContainer::onDeviceWndLoadCurrentSetValue(unsigned int current)
{
    bool ok = device->setLoadCurrent(current);

    logResult(ok,
              "Load current successfully set: [" + QString::number(current) + " mA]",
              "Unable to set load current");
}


bool DeviceContainer::createSubDir(const QString &subDirName, QString &fullPath) {
    QString wsPath = m_AppParamsRef->getParamValue("workspacePath");
    QDir dir(wsPath);

    fullPath = wsPath + "/" + subDirName;

    // Check if the main directory exists
    if (!dir.exists()) return false;

    // Check if the subdirectory already exists
    if (dir.exists(subDirName)) return false;

    // Try to create the subdirectory
    if (!dir.mkpath(subDirName)) return false;

    return true;
}

void DeviceContainer::logResult(bool status, const QString &successMsg, const QString &errorMsg)
{
    if(status)
        log->printLogMessage(successMsg, LOG_MESSAGE_TYPE_INFO);
    else
        log->printLogMessage(errorMsg, LOG_MESSAGE_TYPE_ERROR);
}
