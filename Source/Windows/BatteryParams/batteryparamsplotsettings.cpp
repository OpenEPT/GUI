#include "batteryparamsplotsettings.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSvgGenerator>

batteryparams_plot_settings_t BATTERYPARAMSPLOT_SettingsDefault()
{
    batteryparams_plot_settings_t defaults;

    defaults.fontFamily = BATTERYPARAMSPLOT_FONT_DEFAULT;
    defaults.legendFontFamily = BATTERYPARAMSPLOT_FONT_DEFAULT;
    defaults.labelFontSize = BATTERYPARAMSPLOT_LABEL_SIZE_DEFAULT;
    defaults.tickFontSize = BATTERYPARAMSPLOT_TICK_SIZE_DEFAULT;
    defaults.legendFontSize = BATTERYPARAMSPLOT_LEGEND_SIZE_DEFAULT;
    defaults.lineWidth = BATTERYPARAMSPLOT_LINE_WIDTH_DEFAULT;
    defaults.markerSize = BATTERYPARAMSPLOT_MARKER_SIZE_DEFAULT;
    defaults.gridVisible = true;
    defaults.minorGridVisible = false;
    defaults.legendVisible = true;
    defaults.legendPosition = BATTERYPARAMSPLOT_LEGEND_RIGHT;
    defaults.timeUnit = BATTERYPARAMSPLOT_TIME_MS;

    return defaults;
}

double BATTERYPARAMSPLOT_TimeScale(batteryparams_time_unit_t unit)
{
    switch(unit)
    {
    case BATTERYPARAMSPLOT_TIME_S:   return 1000.0;
    case BATTERYPARAMSPLOT_TIME_MIN: return 60000.0;
    default:                         return 1.0;
    }
}

QString BATTERYPARAMSPLOT_TimeSuffix(batteryparams_time_unit_t unit)
{
    switch(unit)
    {
    case BATTERYPARAMSPLOT_TIME_S:   return "s";
    case BATTERYPARAMSPLOT_TIME_MIN: return "min";
    default:                         return "ms";
    }
}

int BATTERYPARAMSPLOT_TimeDecimals(batteryparams_time_unit_t unit)
{
    switch(unit)
    {
    case BATTERYPARAMSPLOT_TIME_S:   return 4;
    case BATTERYPARAMSPLOT_TIME_MIN: return 6;
    default:                         return 3;
    }
}

/*Applied after the graphs are built, so every analysis window shares the same look*/
void BATTERYPARAMSPLOT_Apply(QCustomPlot *plot, batteryparams_plot_settings_t settings)
{
    QFont labelFont(settings.fontFamily, settings.labelFontSize);
    QFont tickFont(settings.fontFamily, settings.tickFontSize);
    QFont legendFont(settings.legendFontFamily, settings.legendFontSize);
    QCPAxis *axes[3];
    int axisNo = 2;

    if(plot == NULL) return;

    axes[0] = plot->xAxis;
    axes[1] = plot->yAxis;

    if(plot->yAxis2->visible())
    {
        axes[2] = plot->yAxis2;
        axisNo = 3;
    }

    for(int i = 0; i < axisNo; i++)
    {
        axes[i]->setLabelFont(labelFont);
        axes[i]->setTickLabelFont(tickFont);
        axes[i]->grid()->setVisible(settings.gridVisible && (axes[i] != plot->yAxis2));
        axes[i]->grid()->setSubGridVisible(settings.minorGridVisible && (axes[i] != plot->yAxis2));
        axes[i]->setSubTicks(settings.minorGridVisible);
    }

    plot->legend->setFont(legendFont);
    plot->legend->setVisible(settings.legendVisible);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop |
        ((settings.legendPosition == BATTERYPARAMSPLOT_LEGEND_LEFT) ? Qt::AlignLeft : Qt::AlignRight));

    for(int i = 0; i < plot->graphCount(); i++)
    {
        QPen pen = plot->graph(i)->pen();
        QCPScatterStyle scatter = plot->graph(i)->scatterStyle();

        pen.setWidth(settings.lineWidth);
        plot->graph(i)->setPen(pen);

        if(scatter.shape() == QCPScatterStyle::ssNone) continue;

        scatter.setSize(settings.markerSize);
        plot->graph(i)->setScatterStyle(scatter);
    }

    plot->replot();
}

void BATTERYPARAMSPLOT_Save(QCustomPlot *plot, QWidget *parent, QString suggestedName)
{
    QString selectedFilter;
    QString path;

    if(plot == NULL) return;

    path = QFileDialog::getSaveFileName(parent, "Save plot", suggestedName + ".png",
                                        "PNG image (*.png);;SVG image (*.svg)", &selectedFilter);

    if(path.isEmpty()) return;

    if(selectedFilter.startsWith("SVG") && !path.endsWith(".svg", Qt::CaseInsensitive)) path += ".svg";
    if(selectedFilter.startsWith("PNG") && !path.endsWith(".png", Qt::CaseInsensitive)) path += ".png";

    if(path.endsWith(".svg", Qt::CaseInsensitive))
    {
        QSvgGenerator generator;
        QCPPainter painter;

        generator.setFileName(path);
        generator.setSize(plot->size());
        generator.setViewBox(QRect(0, 0, plot->width(), plot->height()));
        generator.setTitle(parent != NULL ? parent->windowTitle() : "OpenEPT plot");

        if(!painter.begin(&generator))
        {
            QMessageBox::warning(parent, "Save plot", "Cannot save " + path);
            return;
        }

        painter.setMode(QCPPainter::pmVectorized);
        painter.setMode(QCPPainter::pmNoCaching);
        plot->toPainter(&painter, plot->width(), plot->height());
        painter.end();
        return;
    }

    if(!plot->savePng(path)) QMessageBox::warning(parent, "Save plot", "Cannot save " + path);
}
