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

class DeviceContainer : public QObject
{
    Q_OBJECT
public:
    explicit DeviceContainer(QObject *parent = nullptr,  DeviceWnd* aDeviceWnd = nullptr, Device* aDevice = nullptr, QString aWsPath="");
    ~DeviceContainer();

signals:
    void    sigDeviceClosed(Device* device);

public slots:
    void    onConsoleWndMessageRcvd(QString msg);
    void    onDeviceWndResolutionChanged(QString resolution);
    void    onDeviceWndADCChanged(QString adc);
    void    onDeviceWndClockDivChanged(QString clockDiv);
    void    onDeviceWndChannelSamplingTimeChanged(QString stime);
    void    onDeviceWndSamplingPeriodChanged(QString time);
    void    onDeviceWndInterfaceChanged(QString interfaceIp);
    void    onDeviceWndAvrRatioChanged(QString avgRatio);
    void    onDeviceWndVOffsetChanged(QString off);
    void    onDeviceWndCOffsetChanged(QString off);



    void    onDeviceWndAcquisitionStart();
    void    onDeviceWndAcquisitionStop();
    void    onDeviceWndAcquisitionPause();
    void    onDeviceWndAdvConfGet();
    void    onDeviceWndAcquisitionRefresh();
    void    onDeviceWndNewConfiguration(QVariant newConfig);
    void    onDeviceWndClosed();
    void    onDeviceWndSaveToFileChanged(bool saveToFile);
    void    onDeviceWndEPEnable(bool aEpEnabled);
    void    onDeviceWndMaxNumberOfBuffersChanged(unsigned int maxNumber);
    void    onDeviceWndConsumptionTypeChanged(QString aConsumptionType);
    void    onDeviceWndMeasurementTypeChanged(QString aMeasurementType);
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

    void    onDeviceControlLinkDisconnected();
    void    onDeviceControlLinkConnected();
    void    onDeviceStatusLinkNewDeviceAdded(QString aDeviceIP);
    void    onDeviceStatusLinkNewMessageReceived(QString aDeviceIP, QString aMessage);
    void    onDeviceHandleControlMsgResponse(QString msg, bool exeStatus);
    void    onDeviceResolutionObtained(QString resolution);
    void    onDeviceClkDivObtained(QString clkDiv);
    void    onDeviceChSampleTimeObtained(QString stime);
    void    onDeviceSamplingPeriodObtained(QString stime);
    void    onDeviceAdcInClkObtained(QString inClk);
    void    onDeviceAvgRatioChanged(QString aAvgRatio);
    void    onDeviceCOffsetObtained(QString coffset);
    void    onDeviceVOffsetObtained(QString voffset);
    void    onDeviceSamplingTimeChanged(double value);
    void    onDeviceLoadStateObtained(bool state);
    void    onDeviceBatStateObtained(bool state);
    void    onDevicePPathStateObtained(bool state);
    void    onDeviceChargerStateObtained(bool state);
    void    onDeviceDACStateObtained(bool state);
    void    onDeviceUVoltageObtained(bool state);
    void    onDeviceOVoltageObtained(bool state);
    void    onDeviceOCurrentObtained(bool state);
    void    onDeviceChargingDone();
    void    onDeviceLoadCurrentObtained(int current);
    void    onDeviceChargerCurrentObtained(int current);
    void    onDeviceChargerTermCurrentObtained(int current);
    void    onDeviceChargerTermVoltageObtained(float voltage);




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



private:
    DeviceWnd*                      deviceWnd;
    Device*                         device;
    Log*                            log;
    FileProcessing*                 fileProcessing;


    device_adc_resolution_t         getAdcResolutionFromString(QString resolution);
    device_adc_clock_div_t          getAdcClockDivFromString(QString clkDiv);
    device_adc_ch_sampling_time_t   getAdcChSamplingTimeFromString(QString chstime);
    device_adc_averaging_t          getAdcAvgRatioFromString(QString avgRatio);
    device_adc_t                    getAdcFromString(QString adc);
    bool                            createSubDir(const QString &subDirName, QString &fullPath);

    double                          elapsedTime;
    QTimer                          *timer;

    QString                         wsPath;

    QString                         consumptionProfileName;
    bool                            consumptionProfileNameSet;
    bool                            consumptionProfileNameExists;
    bool                            globalSaveToFileEnabled; //Mark that for one consumption profile data are stored in file
    bool                            chDschSaveToFileEnabled;
    bool                            writeSamplesToFileEnabled;
    bool                            epEnabled; //

    ChargingState                   chargingState;

};

#endif // DEVICECONTAINER_H
