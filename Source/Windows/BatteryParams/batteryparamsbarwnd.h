#ifndef BATTERYPARAMSBARWND_H
#define BATTERYPARAMSBARWND_H

#include <QLabel>
#include <QVector>
#include <QWidget>

#include "Chart/qcustomplot.h"
#include "batteryparamsplotsettings.h"

typedef struct
{
    unsigned int    index;
    double          value;
}batteryparams_bar_t;

class BatteryParamsBarWnd : public QWidget
{
    Q_OBJECT

public:
    explicit            BatteryParamsBarWnd(QWidget *parent = nullptr);

    void                setData(QString title, QString yLabel, QColor color, QVector<batteryparams_bar_t> bars);
    void                setPlotSettings(batteryparams_plot_settings_t settings);

private slots:
    void                onSaveImage();

private:
    QCustomPlot        *plot;
    QCPBars            *bars;
    QLabel             *summaryLabel;

    batteryparams_plot_settings_t plotSettings;
};

#endif // BATTERYPARAMSBARWND_H
