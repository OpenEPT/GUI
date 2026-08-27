#ifndef DEVICEWND_H
#define DEVICEWND_H

#include <QWidget>
#include <QButtonGroup>
#include <QAbstractButton>
#include "Windows/Plot/plot.h"
#include "Windows/Console/consolewnd.h"
#include "Windows/Device/advcofigurationdata.h"
#include "Windows/Device/datastatistics.h"
#include "Windows/Device/calibrationwnd.h"
#include "Windows/Device/energycontrolwnd.h"
#include "Windows/Device/configurationwnd.h"
#include "Processing/Parameters/deviceparameters.h"

#define     DEVICEWND_DEFAULT_MAX_NUMBER_OF_BUFFERS 100
#define     DEVICEWND_DEFAULT_MAX_NUMBER_OF_SAMPLES 250

typedef enum
{
    DEVICE_STATE_CONNECTED,
    DEVICE_STATE_DISCONNECTED,
    DEVICE_STATE_UNDEFINED
}device_state_t;

typedef enum
{
    DEVICE_ACQ_ACTIVE,
    DEVICE_ACQ_PAUSE,
    DEVICE_ACQ_UNDEFINED
}device_acq_mode_t;

typedef enum
{
    DEVICE_INTERFACE_SELECTION_STATE_UNDEFINED,
    DEVICE_INTERFACE_SELECTION_STATE_SELECTED
}device_interface_selection_state_t;

typedef enum
{
    DEVICE_MODE_INTERNAL,
    DEVICE_MODE_EXTERNAL
}device_mode_t;

typedef enum
{
    DEVICE_CONSUMPTION_TYPE_UNDEF = 0,
    DEVICE_CONSUMPTION_TYPE_CURRENT,
    DEVICE_CONSUMPTION_TYPE_CUMULATIVE
}device_consumption_type_t;

typedef enum
{
    DEVICE_MEASUREMENT_TYPE_UNDEF = 0,
    DEVICE_MEASUREMENT_TYPE_VOLTAGE,
    DEVICE_MEASUREMENT_TYPE_CURRENT
}device_measurement_type_t;

typedef struct
{
    double voltageAvg;
    double voltageMax;
    double voltageMin;
    double currentAvg;
    double currentMax;
    double currentMin;
    double consumptionAvg;
    double consumptionMax;
    double consumptionMin;

}device_stat_info;


namespace Ui {
class DeviceWnd;
}

class DeviceWnd : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceWnd(QWidget *parent = nullptr);
    ~DeviceWnd();

    QPlainTextEdit* getLogWidget();
    void            setParameters(DeviceParameters* params);
    void            setDeviceNetworkState(device_state_t aDeviceState);
    void            setDeviceAcqState(device_acq_mode_t aAcqState);
    void            printConsoleMsg(QString msg, bool exeStatus);
    void            setDeviceInterfaceSelectionState(device_interface_selection_state_t selectionState=DEVICE_INTERFACE_SELECTION_STATE_UNDEFINED);
    bool            setSamplingPeriod(QString stime);
    bool            setLoadState(bool state);
    bool            setPPathState(bool state);
    bool            setBatState(bool state);
    bool            setDACState(bool state);
    bool            setChargerState(bool state);
    bool            setSaveToFileState(bool state);

    void            setChargerConnectionStatus(bool status);

    void            setLoadCurrentStatus(bool state);
    bool            setLoadCurrent(int current);
    void            setChargingCurrentStatus(bool state);

    bool            setUVoltageIndication(bool state);
    bool            setOVoltageIndication(bool state);
    bool            setOCurrentIndication(bool state);

    bool            setUVoltageValue(float value);
    bool            setOVoltageValue(float value);
    bool            setOCurrentValue(int value);

    bool            setBDSize(int value);
    bool            setBDContent(QString content);


    bool            setChargerBDContent(QString content);


    bool            setChargerCurrent(int current);
    bool            setChargerTermCurrent(int current);
    bool            setChargerTermVoltage(float voltage);
    bool            setChargerMaxCurrent(int current);
    bool            setChargerHWSerial(QString serial);
    bool            setChargerFWSerial(QString serial);
    bool            chargingDone();

    bool            setChargingStatus(QString status);

    void            setStatisticsData(double dropRate, unsigned int dropPacketsNo, unsigned int fullReceivedBuffersNo, unsigned int lastBufferID);
    void            setStatisticsSamplingTime(double stime);
    void            setStatisticsElapsedTime(int elapsedTime);

    void            setConfigurationAppliedStatus(bool status);

    void            setConfigurationBDProgressStatus(int percentage, QString status);


    void            setConfigurationChargerBDProgressStatus(int percentage, QString status);

    bool            plotVoltageValues(QVector<double> values, QVector<double> keys);
    bool            plotCurrentValues(QVector<double> values, QVector<double> keys);
    bool            plotConsumptionValues(QVector<double> values, QVector<double> keys);
    bool            plotConsumptionEBP(QVector<double> values, QVector<double> keys);
    bool            plotConsumptionEBPWithName(double value, double key, QString name);
    bool            showStatistic(device_stat_info statInfo);

    bool            setWorkingSpaceDir(QString aWsPath);

    void            setCalibrationData(CalibrationData* data);


    QStringList*    getChSamplingTimeOptions();
    QStringList*    getChAvgRationOptions();
    QStringList*    getClockDivOptions();
    QStringList*    getResolutionOptions();
    QStringList*    getADCOptions();

signals:
    void            sigWndClosed();
    void            sigSamplingPeriodChanged(QString time);
    void            sigSamplesNoChanged(unsigned int newSamplesNo);
    void            sigResolutionChanged(QString resolution);
    void            sigADCChanged(QString adc);
    void            sigClockDivChanged(QString clockDiv);
    void            sigSampleTimeChanged(QString sampleTime);
    void            sigAvrRatioChanged(QString index);
    void            sigVOffsetChanged(QString off);
    void            sigCOffsetChanged(QString off);
    void            sigSaveToFileEnabled(bool enableStatus);
    void            sigEPEnable(bool enableStatus);
    void            sigNewInterfaceSelected(QString interfaceIp);
    void            sigStartAcquisition();
    void            sigPauseAcquisition();
    void            sigStopAcquisition();
    void            sigRefreshAcquisition();
    void            sigConsumptionProfileNameChanged(QString newName);
    void            sigNewControlMessageRcvd(const QString &response);
    void            sigAdvConfigurationReqested();
    void            sigAdvConfigurationChanged(QVariant newConfig);
    void            sigMaxNumberOfBuffersChanged(unsigned int maxNumberOfBuffers);
    void            sigConsumptionTypeChanged(QString consumptionType);
    void            sigMeasurementTypeChanged(QString consumptionType);
    void            sigScatterNameAndKey(QString name, double key);

    void            sigLoadStatusChanged(bool status);
    void            sigLoadCurrentChanged(unsigned int current);
    void            sigLoadCurrentStatusChanged(bool newState);
    void            sigChargingCurrentChanged(unsigned int current);
    void            sigChargingTermCurrentChanged(unsigned int current);
    void            sigChargingTermVoltageChanged(float voltage);
    void            sigChargingCurrentStatusChanged(bool newState);
    void            sigChDschSaveToFileToggled(bool status);
    void            sigPPathStatusChanged(bool status);
    void            sigBatteryStatusChanged(bool status);
    void            sigResetProtection();

    void            sigReadFullBDContent();
    void            sigSetBDContent(QByteArray content);
    void            sigBDFormat();

    void            sigChargerReadFullBDContent();
    void            sigChargerSetBDContent(QByteArray content);
    void            sigChargerBDFormat();

    void            sigCalibrationUpdated();
    void            sigCalibrationStoreRequest();
    void            sigDeviceConfigSet(QMap<QString, QString> changedFields);
    void            sigDeviceConfigGet();
    void            sigDeviceConfigStore();
    void            sigDeviceReset();
protected:
    void            closeEvent(QCloseEvent *event);

public slots:
    void            onDeviceConfigSet(QMap<QString, QString> changedFields);
    void            onDeviceConfigGet();
    void            onDeviceConfigStore();
    void            onDeviceReset();
    void            onSaveToFileChanged(int value);
    void            onStartAcquisition();
    void            onPauseAcquisition();
    void            onStopAcquisiton();
    void            onRefreshAcquisiton();
    void            onConsolePressed();
    void            onDataAnalyzerPressed();
    void            onSetConsumptionName();
    void            onSamplingPeriodChanged();
    void            onInterfaceChanged(QString interfaceInfo);
    void            onAdvConfigurationChanged(QVariant aConfig);
    void            onAdvConfigurationReqested(void);
    void            onMaxNumberOfBuffersChanged();
    void            onEPEnableChanged(int value);
    void            onSamplesNoChanged();


    void            onLoadCurrentStatusChanged(bool newState);
    void            onLoadCurrentChanged(unsigned int current);
    void            onChargingCurrentStatusChanged(bool newState);
    void            onChargingCurrentChanged(unsigned int current);
    void            onChargingTermCurrentChanged(unsigned int current);
    void            onChargingTermVoltageChanged(float voltage);
    void            onLoadStatusChanged(bool status);
    void            onPPathStatusChanged(bool status);
    void            onBatteryStatusChanged(bool status);
    void            onResetProtection();
    void            onChDschSaveToFileChanged(bool state);

    void            onConfWndGetBDContent();
    void            onConfWndSetBDContent(QByteArray content);
    void            onConfWndBDFormat();

    void            onChargerConfWndGetBDContent();
    void            onChargerConfWndSetBDContent(QByteArray content);
    void            onChargerConfWndBDFormat();


    void            onConsumptionProfileNameChanged();

    void            onAdvanceConfigurationButtonPressed(bool pressed);

    void            onCalibrationButtonPressed(bool pressed);

    void            onEnenergyControlButtonPressed(bool pressed);


    void            onNewControlMsgRcvd(QString text);

    void            onPlotScatterNameAndKey(QString name, double key);

private slots:
    void            onCalibrationUpdated();
    void            onCalibrationStoreRequest();

private:
    Ui::DeviceWnd               *ui;

    ConfigurationWnd            *configurationWnd;

    ConsoleWnd                  *consoleWnd;
    DataStatistics              *dataAnalyzer;
    CalibrationWnd              *calibrationWnd;
    EnergyControlWnd            *energyControlWnd;
    Plot                        *voltageChart;
    Plot                        *currentChart;
    Plot                        *consumptionChart;

    QStringList*                adcOptions;
    QStringList*                sampleTimeOptions;
    QStringList*                resolutionOptions;
    QStringList*                clockDivOptions;
    QStringList*                networkInterfacesNames;
    QStringList*                averaginOptions;
    QString                     samplingTime;

    /* File info */
    bool                        saveToFileFlag;
    bool                        consumptionProfileSet;

    bool                        samplingTextChanged = false;
    bool                        voffsetTextChanged = false;

    /* */
    device_state_t                      deviceState;
    device_interface_selection_state_t  interfaceState;
    device_acq_mode_t                   acqState;

    /* */
    void                        setDeviceStateDisconnected();
    void                        setDeviceStateConnected();

    /**/

    QString                     wsPath;

    DeviceParameters           *m_param;
};

#endif // DEVICEWND_H
