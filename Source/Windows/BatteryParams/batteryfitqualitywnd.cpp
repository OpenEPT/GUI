#include "batteryfitqualitywnd.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#define BATTERYFITQUALITY_BAR_WIDTH     0.18

BatteryFitQualityWnd::BatteryFitQualityWnd(QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *controlLayout = new QHBoxLayout();
    QPushButton *saveButton = new QPushButton("Save image", this);

    setFont(defaultFont);
    setWindowTitle("Estimation quality");

    plot = new QCustomPlot(this);
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->legend->setVisible(true);
    plot->legend->setFont(defaultFont);
    plot->xAxis->setLabel("Cycle");
    plot->yAxis->setLabel("Relaxation fit error [mV]");

    barsGroup = new QCPBarsGroup(plot);
    barsGroup->setSpacingType(QCPBarsGroup::stAbsolute);
    barsGroup->setSpacing(1);

    firstOrderRmseBars = barsCreate("1st order RMSE", QColor(120, 170, 220));
    firstOrderMaxBars = barsCreate("1st order max", QColor(30, 110, 200));
    secondOrderRmseBars = barsCreate("2nd order RMSE", QColor(240, 180, 130));
    secondOrderMaxBars = barsCreate("2nd order max", QColor(200, 90, 20));

    firstOrderRmseCheckBox = new QCheckBox("1st order RMSE", this);
    firstOrderMaxCheckBox = new QCheckBox("1st order max", this);
    secondOrderRmseCheckBox = new QCheckBox("2nd order RMSE", this);
    secondOrderMaxCheckBox = new QCheckBox("2nd order max", this);

    firstOrderRmseCheckBox->setChecked(true);
    firstOrderMaxCheckBox->setChecked(true);
    secondOrderRmseCheckBox->setChecked(true);
    secondOrderMaxCheckBox->setChecked(true);

    controlLayout->addWidget(firstOrderRmseCheckBox);
    controlLayout->addWidget(firstOrderMaxCheckBox);
    controlLayout->addWidget(secondOrderRmseCheckBox);
    controlLayout->addWidget(secondOrderMaxCheckBox);
    controlLayout->addStretch();
    controlLayout->addWidget(saveButton);

    summaryLabel = new QLabel(this);
    summaryLabel->setStyleSheet("color: gray;");

    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(plot, 1);
    mainLayout->addWidget(summaryLabel);

    connect(firstOrderRmseCheckBox, &QCheckBox::toggled, this, &BatteryFitQualityWnd::onSeriesToggled);
    connect(firstOrderMaxCheckBox, &QCheckBox::toggled, this, &BatteryFitQualityWnd::onSeriesToggled);
    connect(secondOrderRmseCheckBox, &QCheckBox::toggled, this, &BatteryFitQualityWnd::onSeriesToggled);
    connect(secondOrderMaxCheckBox, &QCheckBox::toggled, this, &BatteryFitQualityWnd::onSeriesToggled);
    connect(saveButton, &QPushButton::clicked, this, &BatteryFitQualityWnd::onSaveImage);

    plotSettings = BATTERYPARAMSPLOT_SettingsDefault();

    resize(1100, 600);
}

QCPBars* BatteryFitQualityWnd::barsCreate(QString name, QColor color)
{
    QCPBars *bars = new QCPBars(plot->xAxis, plot->yAxis);

    bars->setName(name);
    bars->setBarsGroup(barsGroup);
    bars->setWidth(BATTERYFITQUALITY_BAR_WIDTH);
    bars->setPen(QPen(color.darker(130)));
    bars->setBrush(QBrush(color));

    return bars;
}

void BatteryFitQualityWnd::setPlotSettings(batteryparams_plot_settings_t settings)
{
    plotSettings = settings;
    BATTERYPARAMSPLOT_Apply(plot, plotSettings);
}

void BatteryFitQualityWnd::setData(QVector<batteryfitquality_cycle_t> aCycles)
{
    cycles = aCycles;

    updateBars();
    updateSummary();
}

void BatteryFitQualityWnd::updateBars()
{
    QVector<double> keys;
    QVector<double> firstRmse;
    QVector<double> firstMax;
    QVector<double> secondRmse;
    QVector<double> secondMax;
    QSharedPointer<QCPAxisTickerFixed> ticker(new QCPAxisTickerFixed);

    for(int i = 0; i < cycles.size(); i++)
    {
        keys.append(cycles[i].index);
        firstRmse.append(cycles[i].firstOrderValid ? cycles[i].firstOrderRmse : 0);
        firstMax.append(cycles[i].firstOrderValid ? cycles[i].firstOrderMaxError : 0);
        secondRmse.append(cycles[i].secondOrderValid ? cycles[i].secondOrderRmse : 0);
        secondMax.append(cycles[i].secondOrderValid ? cycles[i].secondOrderMaxError : 0);
    }

    firstOrderRmseBars->setData(keys, firstRmse, true);
    firstOrderMaxBars->setData(keys, firstMax, true);
    secondOrderRmseBars->setData(keys, secondRmse, true);
    secondOrderMaxBars->setData(keys, secondMax, true);

    firstOrderRmseBars->setVisible(firstOrderRmseCheckBox->isChecked());
    firstOrderMaxBars->setVisible(firstOrderMaxCheckBox->isChecked());
    secondOrderRmseBars->setVisible(secondOrderRmseCheckBox->isChecked());
    secondOrderMaxBars->setVisible(secondOrderMaxCheckBox->isChecked());

    /*Hidden series are taken out of the legend and out of the scaling, so the
      visible ones fill the plot*/
    firstOrderRmseBars->removeFromLegend();
    firstOrderMaxBars->removeFromLegend();
    secondOrderRmseBars->removeFromLegend();
    secondOrderMaxBars->removeFromLegend();

    if(firstOrderRmseCheckBox->isChecked()) firstOrderRmseBars->addToLegend();
    if(firstOrderMaxCheckBox->isChecked()) firstOrderMaxBars->addToLegend();
    if(secondOrderRmseCheckBox->isChecked()) secondOrderRmseBars->addToLegend();
    if(secondOrderMaxCheckBox->isChecked()) secondOrderMaxBars->addToLegend();

    ticker->setTickStep(1.0);
    ticker->setScaleStrategy(QCPAxisTickerFixed::ssNone);
    plot->xAxis->setTicker(ticker);

    plot->xAxis->rescale();

    {
        bool first = true;
        QCPBars *series[4] = {firstOrderRmseBars, firstOrderMaxBars, secondOrderRmseBars, secondOrderMaxBars};

        for(int i = 0; i < 4; i++)
        {
            if(!series[i]->visible()) continue;
            series[i]->rescaleValueAxis(!first);
            first = false;
        }
    }

    plot->yAxis->setRangeLower(0);
    plot->xAxis->setRange(plot->xAxis->range().lower - 0.5, plot->xAxis->range().upper + 0.5);

    BATTERYPARAMSPLOT_Apply(plot, plotSettings);
}

void BatteryFitQualityWnd::updateSummary()
{
    double firstRmseSum = 0;
    double secondRmseSum = 0;
    double firstMaxWorst = 0;
    double secondMaxWorst = 0;
    int firstNo = 0;
    int secondNo = 0;

    for(int i = 0; i < cycles.size(); i++)
    {
        if(cycles[i].firstOrderValid)
        {
            firstRmseSum += cycles[i].firstOrderRmse;
            firstNo++;
            if(cycles[i].firstOrderMaxError > firstMaxWorst) firstMaxWorst = cycles[i].firstOrderMaxError;
        }

        if(!cycles[i].secondOrderValid) continue;

        secondRmseSum += cycles[i].secondOrderRmse;
        secondNo++;
        if(cycles[i].secondOrderMaxError > secondMaxWorst) secondMaxWorst = cycles[i].secondOrderMaxError;
    }

    if((firstNo == 0) && (secondNo == 0))
    {
        summaryLabel->setText("No cycle could be fitted");
        return;
    }

    summaryLabel->setText(QString("1st order: average RMSE %1 mV, worst deviation %2 mV over %3 cycles   |   "
                                  "2nd order: average RMSE %4 mV, worst deviation %5 mV over %6 cycles")
                          .arg(firstNo > 0 ? firstRmseSum / firstNo : 0, 0, 'f', 3)
                          .arg(firstMaxWorst, 0, 'f', 3)
                          .arg(firstNo)
                          .arg(secondNo > 0 ? secondRmseSum / secondNo : 0, 0, 'f', 3)
                          .arg(secondMaxWorst, 0, 'f', 3)
                          .arg(secondNo));
}

void BatteryFitQualityWnd::onSeriesToggled()
{
    updateBars();
}

void BatteryFitQualityWnd::onSaveImage()
{
    BATTERYPARAMSPLOT_Save(plot, this, "estimation_quality");
}
