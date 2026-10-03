#ifndef BATTERYPARAMSWND_H
#define BATTERYPARAMSWND_H

#include <QWidget>
#include <QComboBox>
#include <QProgressDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVector>

#include "Processing/batteryparamsextraction.h"
#include "batterycyclewnd.h"
#include "batteryparamssettingsdlg.h"
#include "batteryfitintervalwnd.h"
#include "batteryfitqualitywnd.h"
#include "batteryparamsbarwnd.h"
#include "batteryocvanalysiswnd.h"
#include "batteryparamstrendwnd.h"

class BatteryParamsWnd : public QWidget
{
    Q_OBJECT

public:
    explicit            BatteryParamsWnd(QWidget *parent = nullptr);

    void                setProfileName(QString name);
    void                setSettings(batteryparams_settings_t settings);
    void                addCycles(QVector<batteryparams_cycle_t> newCycles, QProgressDialog *progressDialog);

signals:
    void                sigSettingsChanged(batteryparams_settings_t settings);

public slots:
    void                onCycleStarted(unsigned int index, double key);
    void                onCycleFinished(batteryparams_cycle_t cycle);
    void                onClear();

private slots:
    void                onExport();
    void                onSettings();
    void                onResistanceTrend();
    void                onCapacitanceTrend();
    void                onOcvTrend();
    void                onOcvAnalysis();
    void                onFitQuality();
    void                onRelaxationTime();
    void                onFitInterval();
    void                onFitCyclePoints();
    void                onFitPointsSave();
    void                onFitPointsRestore();
    void                onSweepRequested(batteryfitinterval_sweep_t mode, double fixedValue,
                                         double from, double to, int steps, bool compare);
    void                onCycleSelected(int row, int column);
    void                onCycleUpdated(batteryparams_cycle_t cycle);

private:
    QLabel             *statusLabel;
    QPushButton        *settingsButton;
    QPushButton        *resistanceTrendButton;
    QPushButton        *capacitanceTrendButton;
    QPushButton        *ocvTrendButton;
    QPushButton        *ocvAnalysisButton;
    QPushButton        *fitQualityButton;
    QPushButton        *relaxationTimeButton;
    QPushButton        *fitIntervalButton;
    QPushButton        *fitPointsButton;
    QPushButton        *fitPointsSaveButton;
    QPushButton        *fitPointsRestoreButton;
    QLabel             *settingsLabel;

    BatteryParamsSettingsDlg *settingsDlg;
    BatteryParamsTrendWnd    *resistanceTrendWnd;
    BatteryParamsTrendWnd    *capacitanceTrendWnd;
    BatteryParamsTrendWnd    *ocvTrendWnd;
    BatteryOcvAnalysisWnd    *ocvAnalysisWnd;
    BatteryFitQualityWnd     *fitQualityWnd;
    BatteryParamsBarWnd      *relaxationTimeWnd;
    BatteryFitIntervalWnd    *fitIntervalWnd;

    batteryparams_settings_t  settings;
    batteryparams_plot_settings_t plotSettings;
    int                       parallelAnswer;
    QLabel             *summaryLabel;
    QTableWidget       *cyclesTable;
    QPushButton        *clearButton;
    QPushButton        *exportButton;
    BatteryCycleWnd    *cycleWnd;
    QVector<int>        fittedPositions;
    QVector<batteryparams_cycle_t> fittedOriginals;

    void                updateFitPointsButtons();
    bool                connectedToCycleWnd;
    bool                connectedToFitIntervalWnd;

    QString             profileName;
    QVector<batteryparams_cycle_t> cycles;

    QTableWidgetItem*   createItem(QString text);
    void                addCycleRow(batteryparams_cycle_t cycle);
    void                fillCycleRow(int row, batteryparams_cycle_t cycle);
    void                updateSummary();
    void                recalculate();
    bool                parallelGet(int cycleNo);
    void                analyzeCycles(QVector<batteryparams_cycle_t> *target, QProgressDialog *progressDialog, bool parallel);
    void                analyzeCycles(QVector<batteryparams_cycle_t> *target, QProgressDialog *progressDialog, bool parallel, batteryparams_settings_t usedSettings);
    void                updateSettingsLabel();
    batteryparams_trend_t trendCreate(QString name, QColor color);
    void                trendShow(BatteryParamsTrendWnd **window, QString title, QString yLabel, QVector<batteryparams_trend_t> trends);
};

#endif // BATTERYPARAMSWND_H
