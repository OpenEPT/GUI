#ifndef BATTERYPARAMSEXTRACTION_H
#define BATTERYPARAMSEXTRACTION_H

#include <QObject>
#include <QString>
#include <functional>
#include <QVector>

#define BATTERYPARAMS_DEFAULT_PULSE_START_MARKER    "Pulse Start"
#define BATTERYPARAMS_DEFAULT_PULSE_END_MARKER      "Pulse End"
#define BATTERYPARAMS_DEFAULT_PAUSE_START_MARKER    "Pause Start"
#define BATTERYPARAMS_DEFAULT_PAUSE_END_MARKER      "Pause End"

#define BATTERYPARAMS_CYCLE_PLOT_DECIMATION_DEFAULT 10
#define BATTERYPARAMS_RELAX_THRESHOLD_DEFAULT       5.0
#define BATTERYPARAMS_RELAX_WINDOW_DEFAULT          60.0
#define BATTERYPARAMS_RELAX_FILTER_DEFAULT          1000.0
#define BATTERYPARAMS_RELAX_FAST_WINDOW_DIVIDER     60.0
#define BATTERYPARAMS_SETTLING_TAU_NO               5.0
#define BATTERYPARAMS_TAU_GRID_POINTS               60
#define BATTERYPARAMS_TAU_SEPARATION                3.0
#define BATTERYPARAMS_TAU_REFINEMENT_NO             3
#define BATTERYPARAMS_TAU_REFINEMENT_SPAN           1.6
#define BATTERYPARAMS_PEEL_SPLIT                    0.4
#define BATTERYPARAMS_PEEL_BOUNDARY_TAU             3.0
#define BATTERYPARAMS_START_SWEEP_WINDOW            5000.0
#define BATTERYPARAMS_START_SWEEP_MAX_STEPS         40
#define BATTERYPARAMS_INITIAL_SOC_DEFAULT           100.0
#define BATTERYPARAMS_CYCLE_PLOT_STEP_WINDOW        50.0
#define BATTERYPARAMS_OCV_WINDOW_DEFAULT            10
#define BATTERYPARAMS_FIT_AVERAGE_WINDOW_DEFAULT    1
#define BATTERYPARAMS_FIT_MAX_POINTS                4000

typedef enum
{
    BATTERYPARAMS_FIT_END_RELAXATION = 0,
    BATTERYPARAMS_FIT_END_PAUSE
}batteryparams_fit_end_t;

typedef enum
{
    BATTERYPARAMS_MODEL_FIRST_ORDER = 1,
    BATTERYPARAMS_MODEL_SECOND_ORDER = 2
}batteryparams_model_t;

typedef struct
{
    double                  relaxationThreshold;
    double                  relaxationWindow;
    double                  relaxationFilter;  /*ms*/
    batteryparams_model_t   model;
    batteryparams_fit_end_t fitEnd;
    int                     tauGridPoints;
    double                  tauSeparation;
    double                  capacity;
    double                  initialSoc;
    int                     plotDecimation;
    int                     ocvWindow;          /*points averaged for the open circuit voltage*/
    int                     fitAverageWindow;   /*points averaged before the RC fit, 1 keeps raw*/
}batteryparams_settings_t;

typedef struct
{
    unsigned int    index;

    double          pulseStartKey;
    double          pulseEndKey;
    double          pauseStartKey;
    double          pauseEndKey;

    double          pulseEndVoltage;
    double          pulseEndCurrent;
    double          pauseStartVoltage;
    double          pauseStartCurrent;

    double          deltaVoltage;
    double          deltaCurrent;
    double          resistance;

    double          charge;
    double          chargeTotal;
    double          socUsed;
    double          soc;

    bool            resistanceValid;
    bool            socValid;

    bool            relaxationReached;
    double          relaxationKey;
    double          relaxationVoltage;
    double          ocv;

    bool            incomplete;

    batteryparams_model_t model;
    bool            modelValid;
    QString         modelReason;
    double          tauFast;
    double          tauSlow;
    double          fastPortionEndKey;
    double          fitEndKey;
    double          r1;
    double          c1;
    double          r2;
    double          c2;
    bool            r1Valid;
    bool            r2Valid;
    double          fitRmse;
    double          fitMaxError;

    QVector<double> fitKeys;
    QVector<double> fitVoltage;

    QVector<double> relaxKeys;
    QVector<double> relaxVoltage;

    QVector<double> plotKeys;
    QVector<double> plotVoltage;
    QVector<double> plotCurrent;

    QVector<double> avgKeys;
    QVector<double> avgVoltage;
}batteryparams_cycle_t;

Q_DECLARE_METATYPE(batteryparams_cycle_t);

typedef struct
{
    unsigned int    index;
    double          startKey;
    double          endKey;
    double          capacity;
}batteryparams_charge_cycle_t;

Q_DECLARE_METATYPE(batteryparams_charge_cycle_t);

class BatteryParamsExtraction : public QObject
{
    Q_OBJECT
public:
    explicit                BatteryParamsExtraction(QObject *parent = nullptr);

    void                    setMarkerNames(QString pulseStart, QString pulseEnd, QString pauseStart, QString pauseEnd);
    void                    setSamplingPeriod(double aSamplingPeriod);
    void                    setSettings(batteryparams_settings_t aSettings);
    void                    setAnalysisDeferred(bool aDeferred);
    void                    setCaptureIncomplete(bool aCapture);
    void                    flushIncompleteCycle();
    batteryparams_settings_t getSettings();

    static batteryparams_settings_t settingsDefault();
    static void             nameCurrentThread(unsigned int cycleIndex);
    static bool             analyzeCycle(batteryparams_cycle_t *cycle, batteryparams_settings_t settings,
                                         const std::function<void(const QString&)> &progress = std::function<void(const QString&)>());
    static int              relaxationPositionGet(const QVector<double> &keys, const QVector<double> &voltage,
                                                  int start, int end, double threshold, double window);
    static bool             cyclePointsFit(batteryparams_cycle_t *cycle);
    static bool             setPauseStart(batteryparams_cycle_t *cycle, double pauseStartKey);
    static void             seriesSmooth(const QVector<double> &keys, const QVector<double> &values,
                                         double window, QVector<double> *smoothed);
    QVector<batteryparams_cycle_t> getCycles();

signals:
    void                    sigCycleStarted(unsigned int index, double key);
    void                    sigCycleFinished(batteryparams_cycle_t cycle);

public slots:
    void                    onNewSamplesReceived(QVector<double> voltage, QVector<double> current, QVector<double> voltageKeys, QVector<double> currentKeys);
    void                    onNewMarkerReceived(double value, double key, QString name);
    void                    onReset();

private:
    typedef struct
    {
        QString         name;
        double          key;
    }batteryparams_marker_t;

    QString                 pulseStartMarker;
    QString                 pulseEndMarker;
    QString                 pauseStartMarker;
    QString                 pauseEndMarker;

    QVector<double>         voltageSamples;
    QVector<double>         currentSamples;
    QVector<double>         keySamples;
    double                  samplingPeriod;
    batteryparams_settings_t settings;
    bool                    analysisDeferred;
    bool                    captureIncomplete;
    double                  chargeTotal;

    QVector<batteryparams_marker_t> pendingMarkers;
    QVector<batteryparams_cycle_t>  cycles;

    batteryparams_cycle_t   activeCycle;
    bool                    cycleActive;
    unsigned int            cycleCounter;

    int                     samplePositionGet(double key);
    void                    processPendingMarkers();
    void                    processMarker(QString name, int position);
    void                    trimSamples(double key);
    void                    storeCycleSamples();
    void                    finalizeIncompleteCycle(double pauseEndKey);
    void                    cycleReset();
};

#endif // BATTERYPARAMSEXTRACTION_H
