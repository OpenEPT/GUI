#include "batteryparamstrendwnd.h"

#include <QVBoxLayout>

BatteryParamsTrendWnd::BatteryParamsTrendWnd(QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *controlLayout = new QHBoxLayout();

    setFont(defaultFont);
    setWindowTitle("Battery parameters trend");

    seriesLayout = new QHBoxLayout();
    controlLayout->addLayout(seriesLayout);
    controlLayout->addStretch();
    mainLayout->addLayout(controlLayout);

    plot = new Plot(400, 220, false, this);
    plot->setXLabel("SoC [%]");
    plot->setXReversed(true);
    mainLayout->addWidget(plot, 1);

    plotSettings = BATTERYPARAMSPLOT_SettingsDefault();

    resize(900, 550);
}

void BatteryParamsTrendWnd::setTrends(QString title, QString yLabel, QVector<batteryparams_trend_t> trends)
{
    setWindowTitle(title);

    for(int i = 0; i < seriesCheckBoxes.size(); i++)
    {
        seriesLayout->removeWidget(seriesCheckBoxes[i]);
        seriesCheckBoxes[i]->deleteLater();
    }
    seriesCheckBoxes.clear();
    seriesGraphs.clear();
    plot->seriesClear();

    plot->setTitle(title);
    plot->setYLabel(yLabel);
    plot->setXReversed(true);

    for(int i = 0; i < trends.size(); i++)
    {
        QCPGraph *graph = plot->seriesAdd(trends[i].name, trends[i].color, trends[i].soc, trends[i].value);
        QCheckBox *checkBox = new QCheckBox(trends[i].name, this);

        seriesGraphs.append(graph);

        checkBox->setChecked(true);
        checkBox->setStyleSheet("color: " + trends[i].color.name() + ";");
        seriesLayout->addWidget(checkBox);
        seriesCheckBoxes.append(checkBox);

        connect(checkBox, &QCheckBox::toggled, this, &BatteryParamsTrendWnd::onSeriesToggled);
    }

    plot->seriesRescale();
}

void BatteryParamsTrendWnd::setPlotSettings(batteryparams_plot_settings_t settings)
{
    plotSettings = settings;
    plot->applyStyle(plotSettings.fontFamily, plotSettings.labelFontSize, plotSettings.tickFontSize,
                     plotSettings.lineWidth, plotSettings.gridVisible, plotSettings.minorGridVisible);
}

void BatteryParamsTrendWnd::onSeriesToggled()
{
    for(int i = 0; i < seriesCheckBoxes.size() && i < seriesGraphs.size(); i++)
    {
        plot->seriesSetVisible(seriesGraphs[i], seriesCheckBoxes[i]->isChecked());
    }
}
