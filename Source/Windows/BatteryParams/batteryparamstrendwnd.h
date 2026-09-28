#ifndef BATTERYPARAMSTRENDWND_H
#define BATTERYPARAMSTRENDWND_H

#include <QCheckBox>
#include <QColor>
#include <QString>
#include <QVector>
#include <QWidget>

#include "Chart/qcustomplot.h"
#include "batteryparamsplotsettings.h"

typedef struct
{
    QString         name;
    QColor          color;
    QVector<double> soc;
    QVector<double> value;
}batteryparams_trend_t;

class BatteryParamsTrendWnd : public QWidget
{
    Q_OBJECT

public:
    explicit            BatteryParamsTrendWnd(QWidget *parent = nullptr);

    void                setTrends(QString title, QString yLabel, QVector<batteryparams_trend_t> trends);
    void                setPlotSettings(batteryparams_plot_settings_t settings);

private slots:
    void                onSeriesToggled();
    void                onSaveImage();

private:
    QCustomPlot        *plot;
    QHBoxLayout        *seriesLayout;
    QVector<QCheckBox*> seriesCheckBoxes;
    batteryparams_plot_settings_t plotSettings;

    void                rescale();
};

#endif // BATTERYPARAMSTRENDWND_H
