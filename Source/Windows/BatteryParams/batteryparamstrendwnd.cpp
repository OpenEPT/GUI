#include "batteryparamstrendwnd.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#define BATTERYPARAMSTREND_MARKER_SIZE      7

BatteryParamsTrendWnd::BatteryParamsTrendWnd(QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *controlLayout = new QHBoxLayout();
    QPushButton *saveButton = new QPushButton("Save image", this);

    setFont(defaultFont);
    setWindowTitle("Battery parameters trend");

    seriesLayout = new QHBoxLayout();
    controlLayout->addLayout(seriesLayout);
    controlLayout->addStretch();
    controlLayout->addWidget(saveButton);
    mainLayout->addLayout(controlLayout);

    plot = new QCustomPlot(this);
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->legend->setVisible(true);
    plot->legend->setFont(defaultFont);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);
    plot->xAxis->setLabel("SoC [%]");
    plot->xAxis->setRangeReversed(true);
    mainLayout->addWidget(plot, 1);

    connect(saveButton, &QPushButton::clicked, this, &BatteryParamsTrendWnd::onSaveImage);

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
    plot->clearGraphs();

    plot->yAxis->setLabel(yLabel);

    for(int i = 0; i < trends.size(); i++)
    {
        QCPGraph *graph = plot->addGraph();
        QCheckBox *checkBox = new QCheckBox(trends[i].name, this);

        graph->setName(trends[i].name);
        graph->setPen(QPen(trends[i].color, 2));
        graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, trends[i].color,
                                               trends[i].color, BATTERYPARAMSTREND_MARKER_SIZE));
        graph->setData(trends[i].soc, trends[i].value, true);

        checkBox->setChecked(true);
        checkBox->setStyleSheet("color: " + trends[i].color.name() + ";");
        seriesLayout->addWidget(checkBox);
        seriesCheckBoxes.append(checkBox);

        connect(checkBox, &QCheckBox::toggled, this, &BatteryParamsTrendWnd::onSeriesToggled);
    }

    rescale();
}

void BatteryParamsTrendWnd::setPlotSettings(batteryparams_plot_settings_t settings)
{
    plotSettings = settings;
    BATTERYPARAMSPLOT_Apply(plot, plotSettings);
}

void BatteryParamsTrendWnd::onSeriesToggled()
{
    for(int i = 0; i < seriesCheckBoxes.size() && i < plot->graphCount(); i++)
    {
        plot->graph(i)->setVisible(seriesCheckBoxes[i]->isChecked());
        plot->graph(i)->removeFromLegend();
        if(seriesCheckBoxes[i]->isChecked()) plot->graph(i)->addToLegend();
    }

    rescale();
}

void BatteryParamsTrendWnd::rescale()
{
    bool firstVisible = true;

    plot->xAxis->rescale();

    for(int i = 0; i < plot->graphCount(); i++)
    {
        if(!plot->graph(i)->visible()) continue;
        plot->graph(i)->rescaleValueAxis(!firstVisible);
        firstVisible = false;
    }

    if(!firstVisible)
    {
        QCPRange range = plot->yAxis->range();
        plot->yAxis->setRange(range.lower - range.size() * 0.1, range.upper + range.size() * 0.1);
    }

    plot->replot();
}

void BatteryParamsTrendWnd::onSaveImage()
{
    BATTERYPARAMSPLOT_Save(plot, this, windowTitle().toLower().replace(' ', '_'));
}
