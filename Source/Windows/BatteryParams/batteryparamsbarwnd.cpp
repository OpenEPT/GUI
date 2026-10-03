#include "batteryparamsbarwnd.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#define BATTERYPARAMSBAR_WIDTH      0.6

BatteryParamsBarWnd::BatteryParamsBarWnd(QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    QPushButton *saveButton = new QPushButton("Save image", this);

    setFont(defaultFont);
    setWindowTitle("Battery parameters");

    plotSettings = BATTERYPARAMSPLOT_SettingsDefault();

    plot = new QCustomPlot(this);
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->legend->setVisible(false);
    plot->xAxis->setLabel("Cycle");

    bars = new QCPBars(plot->xAxis, plot->yAxis);
    bars->setWidth(BATTERYPARAMSBAR_WIDTH);

    summaryLabel = new QLabel(this);
    summaryLabel->setStyleSheet("color: gray;");

    bottomLayout->addWidget(summaryLabel, 1);
    bottomLayout->addWidget(saveButton);

    mainLayout->addWidget(plot, 1);
    mainLayout->addLayout(bottomLayout);

    connect(saveButton, &QPushButton::clicked, this, &BatteryParamsBarWnd::onSaveImage);

    resize(1000, 550);
}

void BatteryParamsBarWnd::setPlotSettings(batteryparams_plot_settings_t settings)
{
    plotSettings = settings;
    BATTERYPARAMSPLOT_Apply(plot, plotSettings);

    /*A single series needs no legend*/
    plot->legend->setVisible(false);
    plot->replot();
}

void BatteryParamsBarWnd::setData(QString title, QString yLabel, QColor color, QVector<batteryparams_bar_t> aBars)
{
    QVector<double> keys;
    QVector<double> values;
    QSharedPointer<QCPAxisTickerFixed> ticker(new QCPAxisTickerFixed);
    double sum = 0;
    double minimum = 0;
    double maximum = 0;

    setWindowTitle(title);
    plot->yAxis->setLabel(yLabel);

    bars->setPen(QPen(color.darker(130)));
    bars->setBrush(QBrush(color));

    for(int i = 0; i < aBars.size(); i++)
    {
        keys.append(aBars[i].index);
        values.append(aBars[i].value);

        sum += aBars[i].value;
        if((i == 0) || (aBars[i].value < minimum)) minimum = aBars[i].value;
        if((i == 0) || (aBars[i].value > maximum)) maximum = aBars[i].value;
    }

    bars->setData(keys, values, true);

    ticker->setTickStep(1.0);
    ticker->setScaleStrategy(QCPAxisTickerFixed::ssNone);
    plot->xAxis->setTicker(ticker);

    plot->rescaleAxes();
    plot->yAxis->setRangeLower(0);
    plot->xAxis->setRange(plot->xAxis->range().lower - 0.5, plot->xAxis->range().upper + 0.5);

    if(aBars.isEmpty())
    {
        summaryLabel->setText("Nothing to show");
    }
    else
    {
        summaryLabel->setText(QString("%1 cycles, average %2, shortest %3, longest %4")
                              .arg(aBars.size())
                              .arg(sum / aBars.size(), 0, 'f', 1)
                              .arg(minimum, 0, 'f', 1)
                              .arg(maximum, 0, 'f', 1));
    }

    BATTERYPARAMSPLOT_Apply(plot, plotSettings);
    plot->legend->setVisible(false);
    plot->replot();
}

void BatteryParamsBarWnd::onSaveImage()
{
    BATTERYPARAMSPLOT_Save(plot, this, windowTitle().toLower().replace(' ', '_'));
}
