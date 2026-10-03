#ifndef BATTERYPARAMSPLOTSETTINGS_H
#define BATTERYPARAMSPLOTSETTINGS_H

#include <QString>
#include <QWidget>

#include "Chart/qcustomplot.h"

#define BATTERYPARAMSPLOT_FONT_DEFAULT          "Arial"
#define BATTERYPARAMSPLOT_LABEL_SIZE_DEFAULT    10
#define BATTERYPARAMSPLOT_TICK_SIZE_DEFAULT     9
#define BATTERYPARAMSPLOT_LEGEND_SIZE_DEFAULT   9
#define BATTERYPARAMSPLOT_LINE_WIDTH_DEFAULT    2
#define BATTERYPARAMSPLOT_MARKER_SIZE_DEFAULT   7

typedef enum
{
    BATTERYPARAMSPLOT_LEGEND_LEFT = 0,
    BATTERYPARAMSPLOT_LEGEND_RIGHT
}batteryparams_legend_position_t;

typedef enum
{
    BATTERYPARAMSPLOT_TIME_MS = 0,
    BATTERYPARAMSPLOT_TIME_S,
    BATTERYPARAMSPLOT_TIME_MIN
}batteryparams_time_unit_t;

typedef struct
{
    QString                         fontFamily;
    QString                         legendFontFamily;
    int                             labelFontSize;
    int                             tickFontSize;
    int                             legendFontSize;
    int                             lineWidth;
    int                             markerSize;
    bool                            gridVisible;
    bool                            minorGridVisible;
    bool                            legendVisible;
    batteryparams_legend_position_t legendPosition;
    batteryparams_time_unit_t       timeUnit;
}batteryparams_plot_settings_t;

batteryparams_plot_settings_t   BATTERYPARAMSPLOT_SettingsDefault();
double                          BATTERYPARAMSPLOT_TimeScale(batteryparams_time_unit_t unit);
QString                         BATTERYPARAMSPLOT_TimeSuffix(batteryparams_time_unit_t unit);
int                             BATTERYPARAMSPLOT_TimeDecimals(batteryparams_time_unit_t unit);
void                            BATTERYPARAMSPLOT_Apply(QCustomPlot *plot, batteryparams_plot_settings_t settings);
void                            BATTERYPARAMSPLOT_Save(QCustomPlot *plot, QWidget *parent, QString suggestedName);

#endif // BATTERYPARAMSPLOTSETTINGS_H
