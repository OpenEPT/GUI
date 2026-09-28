#ifndef BATTERYFITQUALITYWND_H
#define BATTERYFITQUALITYWND_H

#include <QCheckBox>
#include <QLabel>
#include <QVector>
#include <QWidget>

#include "Chart/qcustomplot.h"
#include "batteryparamsplotsettings.h"

typedef struct
{
    unsigned int    index;
    bool            firstOrderValid;
    double          firstOrderRmse;
    double          firstOrderMaxError;
    bool            secondOrderValid;
    double          secondOrderRmse;
    double          secondOrderMaxError;
}batteryfitquality_cycle_t;

class BatteryFitQualityWnd : public QWidget
{
    Q_OBJECT

public:
    explicit            BatteryFitQualityWnd(QWidget *parent = nullptr);

    void                setData(QVector<batteryfitquality_cycle_t> cycles);
    void                setPlotSettings(batteryparams_plot_settings_t settings);

private slots:
    void                onSeriesToggled();
    void                onSaveImage();

private:
    QCustomPlot        *plot;
    QCPBars            *firstOrderRmseBars;
    QCPBars            *firstOrderMaxBars;
    QCPBars            *secondOrderRmseBars;
    QCPBars            *secondOrderMaxBars;
    QCPBarsGroup       *barsGroup;

    QCheckBox          *firstOrderRmseCheckBox;
    QCheckBox          *firstOrderMaxCheckBox;
    QCheckBox          *secondOrderRmseCheckBox;
    QCheckBox          *secondOrderMaxCheckBox;
    QLabel             *summaryLabel;

    QVector<batteryfitquality_cycle_t> cycles;
    batteryparams_plot_settings_t plotSettings;

    QCPBars*            barsCreate(QString name, QColor color);
    void                updateBars();
    void                updateSummary();
};

#endif // BATTERYFITQUALITYWND_H
