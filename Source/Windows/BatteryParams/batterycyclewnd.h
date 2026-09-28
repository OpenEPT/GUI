#ifndef BATTERYCYCLEWND_H
#define BATTERYCYCLEWND_H

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include "Processing/batteryparamsextraction.h"
#include "Windows/BatteryParams/batteryparamsplotsettings.h"
#include "Windows/Plot/plot.h"

class BatteryCycleWnd : public QWidget
{
    Q_OBJECT

public:
    explicit            BatteryCycleWnd(QWidget *parent = nullptr);

    void                setCycle(batteryparams_cycle_t aCycle);
    void                setPlotSettings(batteryparams_plot_settings_t settings);

signals:
    void                sigCycleUpdated(batteryparams_cycle_t cycle);

private slots:
    void                onMarkerStepBack();
    void                onMarkerStepForward();
    void                onMarkersEdited();
    void                onRestore();
    void                onSave();

private:
    QLabel             *infoLabel;
    QLabel             *modelLabel;
    Plot               *voltagePlot;
    Plot               *currentPlot;

    QLineEdit          *pulseStartEdit;
    QLineEdit          *pulseEndEdit;
    QLineEdit          *pauseStartEdit;
    QLineEdit          *pauseEndEdit;
    QPushButton        *saveButton;
    QPushButton        *restoreButton;

    batteryparams_plot_settings_t plotSettings;
    batteryparams_cycle_t cycle;
    batteryparams_cycle_t originalCycle;

    void                addMarker(Plot *plot, QVector<double> values, double key, QString name);
    int                 keyPositionGet(QVector<double> keys, double key);
    QHBoxLayout*        createMarkerRow(QString name, QLineEdit **edit);
    void                markerStep(QLineEdit *edit, int step);
    void                applyMarkers();
    void                updateInfo();
    void                updatePlots();
};

#endif // BATTERYCYCLEWND_H
