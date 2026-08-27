#ifndef DEVICECONTAINER_H
#define DEVICECONTAINER_H

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QDir>
#include "device.h"
#include "Windows/Device/devicewnd.h"
#include "Utility/log.h"
#include "Processing/fileprocessing.h"
#include "Processing/charginganalysis.h"
#include "Processing/Parameters/applicationparameters.h"

class DeviceContainer : public QObject
{
    Q_OBJECT
public:
    explicit DeviceContainer(QObject *parent = nullptr,
                             DeviceWnd* aDeviceWnd = nullptr,
                             Device* aDevice = nullptr,
                             ApplicationParameters* appParam=nullptr,
                             QDockWidget* dock=nullptr);
    ~DeviceContainer();

    bool getDeviceName(QString* name);
    Device* getDevice();
    DeviceWnd* getDeviceWnd();
signals:
    void    sigDeviceClosed(DeviceContainer* container);

public slots:
    void    onConsoleWndMessageRcvd(QString msg);
    void    onDeviceWndSamplesNoChanged(unsigned int newSamplesNo);
    void    onDeviceWndSamplingPeriodChanged(QString time);
    void    onDeviceWndInterfaceChanged(QString interfaceIp);



    void    onDeviceWndAcquisitionStart();
    void    onDeviceWndAcquisitionStop();
    void    onDeviceWndAcquisitionPause();
    void    onDeviceWndAcquisitionRefresh();


    void    onDeviceWndClosed();
    void    onDeviceWndSaveToFileChanged(bool saveToFile);
    void    onDeviceWndEPEnable(bool aEpEnabled);
    void    onDeviceWndMaxNumberOfBuffersChanged(unsigned int maxNumber);
    void    onDeviceWndConsumptionProfileNameChanged(QString aConsumptionProfileName);

    void    onDeviceWndLoadStatusChanged(bool status);
    void    onDeviceWndPPathStatusChanged(bool status);
    void    onDeviceWndBatteryStatusChanged(bool status);
    void    onDeviceWndResetProtection();

    void    onDeviceWndLoadCurrentSetValue(unsigned int current);
    void    onDeviceWndLoadCurrentSetStatus(bool status);
    void    onDeviceWndChargingCurrentSetValue(unsigned int current);
    void    onDeviceWndChargingTermCurrentSetValue(unsigned int current);
    void    onDeviceWndChargingTermVoltageSetValue(float voltage);
    void    onDeviceWndChargingCurrentSetStatus(bool status);
    void    onDeviceWndChDschSaveToFileChanged(bool saveToFile);
    void    onDeviceWndGetBDContent();
    void    onDeviceWndSetBDContent(QByteArray content);
    void    onDeviceWndBDFormat();
    void    onDeviceWndChargerGetBDContent();
    void    onDeviceWndChargerSetBDContent(QByteArray content);
    void    onDeviceWndChargerBDFormat();


    void    onDeviceControlLinkDisconnected();
    void    onDeviceControlLinkConnected();
    void    onDeviceStatusLinkNewDeviceAdded(QString aDeviceIP);
    void    onDeviceStatusLinkNewMessageReceived(QString aDeviceIP, QString aMessage);
    void    onDeviceHandleControlMsgResponse(QString msg, bool exeStatus);
    void    onDeviceSamplingPeriodObtained(QString stime);
    void    onDeviceSamplingTimeChanged(double value);

    void    onDeviceLoadStateObtained(bool state);
    void    onDeviceBatStateObtained(bool state);
    void    onDevicePPathStateObtained(bool state);
    void    onDeviceChargerStateObtained(bool state);
    void    onDeviceDACStateObtained(bool state);
    void    onDeviceUVoltageObtained(bool state);
    void    onDeviceChargerConnectionStatusOntained(bool state);
    void    onDeviceOVoltageObtained(bool state);
    void    onDeviceOCurrentObtained(bool state);
    void    onDeviceUVoltageValueObtained(float value);
    void    onDeviceOVoltageValueObtained(float value);
    void    onDeviceOCurrentValueObtained(int value);
    void    onDeviceBDSizeObtained(int value);

    void    onDeviceConfigUpdate(QMap<QString, QString> changedFields);
    void    onDeviceConfigGet();
    void    onDeviceConfigStore();
    void    onDeviceReset();

    void    onDeviceBDChunkRead(float percentage);
    void    onDeviceBDChunkWrite(float percentage);

    void    onDeviceChargingDone();
    void    onDeviceLoadCurrentObtained(int current);
    void    onDeviceChargerCurrentObtained(int current);
    void    onDeviceChargerTermCurrentObtained(int current);
    void    onDeviceChargerTermVoltageObtained(float voltage);
    void    onDeviceChargerMaxCurrentObtained(int current);
    void    onDeviceChargerHWSerialObtained(QString serial);
    void    onDeviceChargerFWVersionObtained(QString serial);




    void    onDeviceAcquisitonStarted();
    void    onDeviceAcquisitonStopped();

    void    onTimeout();

    void    onDeviceNewVoltageCurrentSamplesReceived(QVector<double> voltage, QVector<double> current, QVector<double> voltageKeys, QVector<double> currentKeys);
    void    onDeviceNewConsumptionDataReceived(QVector<double> consumption, QVector<double> keys, dataprocessing_consumption_mode_t mode);
    void    onDeviceNewStatisticsReceived(dataprocessing_dev_info_t voltageStat, dataprocessing_dev_info_t currentStat, dataprocessing_dev_info_t consumptionStat);
    void    onDeviceNewSamplesBuffersProcessingStatistics(double dropRate, unsigned int dropPacketsNo, unsigned int fullReceivedBuffersNo, unsigned int lastBufferID, unsigned short ebp);
    void    onDeviceNewEBP(QVector<double> ebpValues, QVector<double> keys);
    void    onDeviceNewEBPFull(double value, double key, QString name);

    void    onDeviceMeasurementEnergyFlowStatusChanged(charginganalysis_status_t status);

private slots:
    void    onDeviceWndCalibrationUpdated();
    void    onDeviceWndCalibrationStoreRequest();



private:
    DeviceWnd*                      deviceWnd;
    Device*                         device;
    Log*                            log;
    FileProcessing*                 fileProcessing;

    void                            fillDeviceSetFunctions();
    bool                            createSubDir(const QString &subDirName, QString &fullPath);

    void                            logResult(bool status, const QString& successMsg, const QString& errorMsg);

    double                          elapsedTime;
    QTimer                          *timer;


    QString                         consumptionProfileName;
    bool                            consumptionProfileNameSet;
    bool                            consumptionProfileNameExists;
    bool                            globalSaveToFileEnabled; //Mark that for one consumption profile data are stored in file
    bool                            chDschSaveToFileEnabled;
    bool                            writeSamplesToFileEnabled;
    bool                            epEnabled; //

    ChargingState                   chargingState;

    ApplicationParameters           *m_AppParamsRef;

    QDockWidget*                    m_dock;


    QMap<QString, std::function<void(const QString&)>> deviceSetHandlers;

};

#endif // DEVICECONTAINER_H
