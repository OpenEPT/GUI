#ifndef BATTERYPARAMSWND_H
#define BATTERYPARAMSWND_H

#include <QWidget>
#include <QComboBox>
#include <QProgressDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QVector>
#include <QSet>
#include <QPair>
#include <QAtomicInt>

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
    void                setExtractionRunning(bool running);
    void                setLiveStatus(double amplitude, double pauseMs, bool dvdt,
                                      double socHigh, double socLow, double soc, QString nextText);
    void                clearLiveStatus();
    void                addCycles(QVector<batteryparams_cycle_t> newCycles, QProgressDialog *progressDialog);
    void                addChargeCycles(QVector<batteryparams_charge_cycle_t> newCharges);

signals:
    void                sigSettingsChanged(batteryparams_settings_t settings);
    void                sigPointsStored(QVector<QPair<QString, double>> markers);

public slots:
    void                onCycleStarted(unsigned int index, double key);
    void                onCycleFinished(batteryparams_cycle_t cycle);
    void                onChargeCycleFinished(batteryparams_charge_cycle_t cycle);
    void                onClear();

private slots:
    void                onExport();
    void                onDeleteCycle();
    void                onStatistics();
    void                onStoreNewPoints();
    void                onSettings();
    void                onGraph();
    void                onAnalysisWindow();
    void                onColumnToggled();
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
    void                onFindNewStartPoint();
    void                onSweepRequested(batteryfitinterval_sweep_t mode, double fixedValue,
                                         double from, double to, int steps, bool compare);
    void                onCycleSelected(int row, int column);
    void                onCycleContextMenu(const QPoint &pos);
    void                onSaveNewCycles();
    void                onCycleUpdated(batteryparams_cycle_t cycle);

private:
    QLabel             *statusLabel;
    QLabel             *liveStatusLabel;

    QAction            *configAction;
    QAction            *statisticsAction;
    QAction            *graphAction;
    QAction            *analysisAction;
    QAction            *fitPointsAction;
    QAction            *fitPointsSaveAction;
    QAction            *fitPointsRestoreAction;
    QAction            *fitPointsFindStartAction;
    QAction            *recalcAction;
    QMenu              *columnsMenu;
    QVector<QAction*>   columnActions;

    QWidget                  *graphWnd;
    BatteryParamsTrendWnd    *graphResistance;
    BatteryParamsTrendWnd    *graphCapacitance;
    BatteryParamsTrendWnd    *graphOcv;
    BatteryParamsTrendWnd    *graphRelaxation;

    QWidget            *analysisWnd;

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
    bool                      extractionRunning;
    batteryparams_plot_settings_t plotSettings;
    int                       parallelAnswer;
    QLabel             *summaryLabel;
    QTabWidget         *cycleTabs;
    QTableWidget       *cyclesTable;
    QTableWidget       *chargeCyclesTable;
    QVector<batteryparams_charge_cycle_t> chargeCycles;
    QPushButton        *clearButton;
    QPushButton        *exportButton;
    QPushButton        *saveCyclesButton;
    QPushButton        *storeCyclesButton;
    QSet<int>           deletedCycleRows;
    QVector<double>     removedMarkerKeys;
    bool                fitInProgress;
    QAtomicInt          analyzeCancel;
    QVector<batteryparams_cycle_t> pendingCycles;
    BatteryCycleWnd    *cycleWnd;
    QVector<int>        fittedPositions;
    QVector<batteryparams_cycle_t> fittedOriginals;

    void                updateFitPointsButtons();
    void                refreshGraphs();
    QVector<batteryparams_trend_t> buildResistanceTrends();
    QVector<batteryparams_trend_t> buildCapacitanceTrends();
    QVector<batteryparams_trend_t> buildOcvTrends();
    QVector<batteryparams_trend_t> buildRelaxationTrends();
    bool                connectedToCycleWnd;
    bool                connectedToFitIntervalWnd;

    QString             profileName;
    QVector<batteryparams_cycle_t> cycles;

    QTableWidgetItem*   createItem(QString text);
    void                addCycleRow(batteryparams_cycle_t cycle);
    void                addChargeCycleRow(batteryparams_charge_cycle_t cycle);
    void                openCycleData(int row);
    void                deleteCycleAt(int row);
    void                fitCycleAt(int row);
    void                endFit();
    QVector<QPair<QString, double>> collectCycleMarkers();
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
