#ifndef BATTERYFITINTERVALWND_H
#define BATTERYFITINTERVALWND_H

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSlider>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QVector>
#include <QWidget>

#include "Chart/qcustomplot.h"
#include "batteryparamsplotsettings.h"

typedef enum
{
    BATTERYFITINTERVAL_SWEEP_THRESHOLD = 0,
    BATTERYFITINTERVAL_SWEEP_WINDOW
}batteryfitinterval_sweep_t;

typedef struct
{
    unsigned int    index;

    bool            relaxationValid;
    double          relaxationR1;
    double          relaxationC1;
    double          relaxationR2;
    double          relaxationC2;
    double          relaxationRmse;
    double          relaxationMaxError;

    bool            pauseValid;
    double          pauseR1;
    double          pauseC1;
    double          pauseR2;
    double          pauseC2;
    double          pauseRmse;
    double          pauseMaxError;
}batteryfitinterval_cycle_t;

class BatteryFitIntervalWnd : public QWidget
{
    Q_OBJECT

public:
    explicit            BatteryFitIntervalWnd(QWidget *parent = nullptr);

    void                setData(QVector<batteryfitinterval_cycle_t> cycles);
    void                setPlotSettings(batteryparams_plot_settings_t settings);
    void                setSweep(QVector<double> sweepValues, QVector<double> rmse, QVector<double> maxError,
                                 QVector<double> relaxedNo, double referenceRmse, double referenceMaxError,
                                 QVector<unsigned int> cycleIndexes,
                                 QVector<QVector<double>> cycleRmse, QVector<QVector<double>> cycleMaxError,
                                 QVector<double> relaxationTime, QVector<QVector<double>> cycleRelaxationTime);

signals:
    void                sigSweepRequested(batteryfitinterval_sweep_t mode, double fixedValue,
                                          double from, double to, int steps, bool compare);

private slots:
    void                onSeriesToggled();
    void                onSweepModeChanged();
    void                onSweepRun();
    void                onSweepSliderMoved(int position);
    void                onSweepViewChanged();
    void                onSaveImage();

private:
    QTabWidget         *tabWidget;
    QCustomPlot        *errorPlot;
    QCustomPlot        *differencePlot;
    QTableWidget       *table;

    QVector<QCPBars*>   errorBars;
    QVector<QCPBars*>   differenceBars;
    QVector<QCheckBox*> errorCheckBoxes;
    QVector<QCheckBox*> differenceCheckBoxes;
    QCPBarsGroup       *errorGroup;
    QCPBarsGroup       *differenceGroup;
    QLabel             *summaryLabel;

    QCustomPlot        *sweepPlot;
    QComboBox          *sweepModeCombo;
    QLineEdit          *sweepFixedEdit;
    QLabel             *sweepFixedUnitLabel;
    QLineEdit          *sweepFromEdit;
    QLineEdit          *sweepToEdit;
    QLineEdit          *sweepStepsEdit;
    QCheckBox          *sweepCompareCheckBox;
    QComboBox          *sweepViewCombo;
    QSlider            *sweepSlider;
    QLabel             *sweepPointLabel;
    QCPItemStraightLine *sweepMarker;

    QVector<double>     sweepValues;
    QVector<double>     sweepRmse;
    QVector<double>     sweepMaxError;
    QVector<double>     sweepRelaxedNo;
    QVector<unsigned int> sweepCycleIndexes;
    QVector<QVector<double>> sweepCycleRmse;
    QVector<QVector<double>> sweepCycleMaxError;
    QVector<double>     sweepRelaxationTime;
    QVector<QVector<double>> sweepCycleRelaxationTime;
    double              sweepReferenceRmse;
    double              sweepReferenceMaxError;

    QVector<batteryfitinterval_cycle_t> cycles;
    batteryparams_plot_settings_t plotSettings;

    QCPBars*            barsCreate(QCustomPlot *plot, QCPBarsGroup *group, QString name, QColor color);
    void                rescale(QCustomPlot *plot, QVector<QCPBars*> &bars, QVector<QCheckBox*> &checkBoxes);
    void                updatePlots();
    void                updateTable();
    void                updateSummary();
    void                sweepTabCreate(QWidget *parent);
    void                sweepPlotUpdate();
};

#endif // BATTERYFITINTERVALWND_H
