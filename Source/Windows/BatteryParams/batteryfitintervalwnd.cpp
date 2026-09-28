#include "batteryfitintervalwnd.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <math.h>

#define BATTERYFITINTERVAL_BAR_WIDTH    0.2

static QTableWidgetItem* prvBATTERYFITINTERVAL_ItemCreate(QString text)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);

    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);

    return item;
}

static double prvBATTERYFITINTERVAL_Difference(double reference, double value)
{
    if(fabs(reference) < 1e-12) return 0;

    return (value - reference) / reference * 100.0;
}

BatteryFitIntervalWnd::BatteryFitIntervalWnd(QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    QWidget *errorTab = new QWidget(this);
    QVBoxLayout *errorLayout = new QVBoxLayout(errorTab);
    QHBoxLayout *errorControlLayout = new QHBoxLayout();
    QWidget *differenceTab = new QWidget(this);
    QVBoxLayout *differenceLayout = new QVBoxLayout(differenceTab);
    QHBoxLayout *differenceControlLayout = new QHBoxLayout();
    QPushButton *saveButton = new QPushButton("Save image", this);
    QStringList header;

    setFont(defaultFont);
    setWindowTitle("Fit interval comparison");

    plotSettings = BATTERYPARAMSPLOT_SettingsDefault();
    tabWidget = new QTabWidget(this);

    errorPlot = new QCustomPlot(errorTab);
    errorPlot->setInteraction(QCP::iRangeDrag, true);
    errorPlot->setInteraction(QCP::iRangeZoom, true);
    errorPlot->legend->setVisible(true);
    errorPlot->xAxis->setLabel("Cycle");
    errorPlot->yAxis->setLabel("Fit error [mV]");
    errorGroup = new QCPBarsGroup(errorPlot);
    errorGroup->setSpacingType(QCPBarsGroup::stAbsolute);
    errorGroup->setSpacing(1);

    errorBars.append(barsCreate(errorPlot, errorGroup, "To relaxation: RMSE", QColor(150, 200, 150)));
    errorBars.append(barsCreate(errorPlot, errorGroup, "To relaxation: max", QColor(30, 150, 60)));
    errorBars.append(barsCreate(errorPlot, errorGroup, "To pause end: RMSE", QColor(240, 180, 130)));
    errorBars.append(barsCreate(errorPlot, errorGroup, "To pause end: max", QColor(200, 90, 20)));

    for(int i = 0; i < errorBars.size(); i++)
    {
        QCheckBox *checkBox = new QCheckBox(errorBars[i]->name(), errorTab);

        checkBox->setChecked(true);
        errorCheckBoxes.append(checkBox);
        errorControlLayout->addWidget(checkBox);
        connect(checkBox, &QCheckBox::toggled, this, &BatteryFitIntervalWnd::onSeriesToggled);
    }
    errorControlLayout->addStretch();
    errorLayout->addLayout(errorControlLayout);
    errorLayout->addWidget(errorPlot, 1);
    tabWidget->addTab(errorTab, "Fit error");

    differencePlot = new QCustomPlot(differenceTab);
    differencePlot->setInteraction(QCP::iRangeDrag, true);
    differencePlot->setInteraction(QCP::iRangeZoom, true);
    differencePlot->legend->setVisible(true);
    differencePlot->xAxis->setLabel("Cycle");
    differencePlot->yAxis->setLabel("Difference to relaxation point fit [%]");
    differenceGroup = new QCPBarsGroup(differencePlot);
    differenceGroup->setSpacingType(QCPBarsGroup::stAbsolute);
    differenceGroup->setSpacing(1);

    differenceBars.append(barsCreate(differencePlot, differenceGroup, "R1", QColor(200, 60, 20)));
    differenceBars.append(barsCreate(differencePlot, differenceGroup, "C1", QColor(30, 110, 200)));
    differenceBars.append(barsCreate(differencePlot, differenceGroup, "R2", QColor(30, 150, 60)));
    differenceBars.append(barsCreate(differencePlot, differenceGroup, "C2", QColor(120, 60, 180)));

    for(int i = 0; i < differenceBars.size(); i++)
    {
        QCheckBox *checkBox = new QCheckBox(differenceBars[i]->name(), differenceTab);

        checkBox->setChecked(true);
        differenceCheckBoxes.append(checkBox);
        differenceControlLayout->addWidget(checkBox);
        connect(checkBox, &QCheckBox::toggled, this, &BatteryFitIntervalWnd::onSeriesToggled);
    }
    differenceControlLayout->addStretch();
    differenceLayout->addLayout(differenceControlLayout);
    differenceLayout->addWidget(differencePlot, 1);
    tabWidget->addTab(differenceTab, "Parameter difference");

    header << "Cycle"
           << "R1 relax [mOhm]" << "R1 pause [mOhm]" << "dR1 [%]"
           << "C1 relax [F]" << "C1 pause [F]" << "dC1 [%]"
           << "R2 relax [mOhm]" << "R2 pause [mOhm]" << "dR2 [%]"
           << "C2 relax [F]" << "C2 pause [F]" << "dC2 [%]"
           << "RMSE relax [mV]" << "RMSE pause [mV]"
           << "Max relax [mV]" << "Max pause [mV]";
    table = new QTableWidget(0, header.size(), this);
    table->setHorizontalHeaderLabels(header);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->horizontalHeader()->setSectionsMovable(true);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(20);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tabWidget->addTab(table, "Table");

    QWidget *sweepTab = new QWidget(this);

    sweepTabCreate(sweepTab);
    tabWidget->addTab(sweepTab, "Relaxation sweep");

    summaryLabel = new QLabel(this);
    summaryLabel->setStyleSheet("color: gray;");

    bottomLayout->addWidget(summaryLabel, 1);
    bottomLayout->addWidget(saveButton);

    mainLayout->addWidget(tabWidget, 1);
    mainLayout->addLayout(bottomLayout);

    connect(saveButton, &QPushButton::clicked, this, &BatteryFitIntervalWnd::onSaveImage);

    resize(1250, 700);
}

QCPBars* BatteryFitIntervalWnd::barsCreate(QCustomPlot *plot, QCPBarsGroup *group, QString name, QColor color)
{
    QCPBars *bars = new QCPBars(plot->xAxis, plot->yAxis);

    bars->setName(name);
    bars->setBarsGroup(group);
    bars->setWidth(BATTERYFITINTERVAL_BAR_WIDTH);
    bars->setPen(QPen(color.darker(130)));
    bars->setBrush(QBrush(color));

    return bars;
}

void BatteryFitIntervalWnd::setPlotSettings(batteryparams_plot_settings_t settings)
{
    plotSettings = settings;

    BATTERYPARAMSPLOT_Apply(errorPlot, plotSettings);
    BATTERYPARAMSPLOT_Apply(differencePlot, plotSettings);
    BATTERYPARAMSPLOT_Apply(sweepPlot, plotSettings);
}

/*Relaxation is decided by how much the voltage is allowed to move (dV) over how
  long (dt). One of the two is held and the other swept, so it is visible how the
  fit error follows the criterion*/
void BatteryFitIntervalWnd::sweepTabCreate(QWidget *parent)
{
    QVBoxLayout *layout = new QVBoxLayout(parent);
    QHBoxLayout *controlLayout = new QHBoxLayout();
    QHBoxLayout *sliderLayout = new QHBoxLayout();
    QPushButton *runButton = new QPushButton("Run sweep", parent);

    sweepModeCombo = new QComboBox(parent);
    sweepModeCombo->addItem("Sweep dV, hold dt");
    sweepModeCombo->addItem("Sweep dt, hold dV");
    sweepModeCombo->setToolTip("Which half of the relaxation criterion is swept");

    sweepFixedEdit = new QLineEdit("60", parent);
    sweepFixedEdit->setFixedWidth(70);
    sweepFixedUnitLabel = new QLabel("[s]", parent);

    sweepFromEdit = new QLineEdit("1", parent);
    sweepFromEdit->setFixedWidth(70);
    sweepToEdit = new QLineEdit("20", parent);
    sweepToEdit->setFixedWidth(70);
    sweepStepsEdit = new QLineEdit("10", parent);
    sweepStepsEdit->setFixedWidth(70);

    sweepViewCombo = new QComboBox(parent);
    sweepViewCombo->addItem("Average over cycles");
    sweepViewCombo->addItem("Per cycle: RMSE");
    sweepViewCombo->addItem("Per cycle: max error");
    sweepViewCombo->addItem("Per cycle: relaxation time");
    sweepViewCombo->setToolTip("Average of all cycles, or one curve per cycle");

    sweepCompareCheckBox = new QCheckBox("Compare with pause end fit", parent);
    sweepCompareCheckBox->setChecked(true);
    sweepCompareCheckBox->setToolTip("Draws the error of the fit over the whole pause as a reference line");

    controlLayout->addWidget(sweepModeCombo);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(new QLabel("Hold", parent));
    controlLayout->addWidget(sweepFixedEdit);
    controlLayout->addWidget(sweepFixedUnitLabel);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(new QLabel("From", parent));
    controlLayout->addWidget(sweepFromEdit);
    controlLayout->addWidget(new QLabel("to", parent));
    controlLayout->addWidget(sweepToEdit);
    controlLayout->addWidget(new QLabel("steps", parent));
    controlLayout->addWidget(sweepStepsEdit);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(sweepViewCombo);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(sweepCompareCheckBox);
    controlLayout->addStretch();
    controlLayout->addWidget(runButton);
    layout->addLayout(controlLayout);

    sweepPlot = new QCustomPlot(parent);
    sweepPlot->setInteraction(QCP::iRangeDrag, true);
    sweepPlot->setInteraction(QCP::iRangeZoom, true);
    sweepPlot->legend->setVisible(true);
    sweepPlot->xAxis->setLabel("Relaxation threshold dV [mV]");
    sweepPlot->yAxis->setLabel("Fit error over the whole pause [mV]");
    layout->addWidget(sweepPlot, 1);

    sweepMarker = new QCPItemStraightLine(sweepPlot);
    sweepMarker->setPen(QPen(QColor(120, 120, 120), 1, Qt::DashLine));
    sweepMarker->setVisible(false);

    sweepSlider = new QSlider(Qt::Horizontal, parent);
    sweepSlider->setEnabled(false);
    sweepPointLabel = new QLabel("Run the sweep to see how the error follows the criterion", parent);

    sliderLayout->addWidget(sweepSlider, 1);
    sliderLayout->addWidget(sweepPointLabel);
    layout->addLayout(sliderLayout);

    connect(sweepModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BatteryFitIntervalWnd::onSweepModeChanged);
    connect(runButton, &QPushButton::clicked, this, &BatteryFitIntervalWnd::onSweepRun);
    connect(sweepSlider, &QSlider::valueChanged, this, &BatteryFitIntervalWnd::onSweepSliderMoved);
    connect(sweepViewCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BatteryFitIntervalWnd::onSweepViewChanged);
}

void BatteryFitIntervalWnd::onSweepModeChanged()
{
    bool thresholdSwept = (sweepModeCombo->currentIndex() == 0);

    sweepFixedUnitLabel->setText(thresholdSwept ? "[s]" : "[mV]");
    sweepFixedEdit->setText(thresholdSwept ? "60" : "5");
    sweepFromEdit->setText(thresholdSwept ? "1" : "10");
    sweepToEdit->setText(thresholdSwept ? "20" : "600");
    sweepPlot->xAxis->setLabel(thresholdSwept ? "Relaxation threshold dV [mV]" : "Relaxation window dt [s]");
    sweepPlot->replot();
}

void BatteryFitIntervalWnd::onSweepRun()
{
    batteryfitinterval_sweep_t mode = (sweepModeCombo->currentIndex() == 0) ?
                                      BATTERYFITINTERVAL_SWEEP_THRESHOLD : BATTERYFITINTERVAL_SWEEP_WINDOW;

    emit sigSweepRequested(mode, sweepFixedEdit->text().toDouble(),
                           sweepFromEdit->text().toDouble(), sweepToEdit->text().toDouble(),
                           sweepStepsEdit->text().toInt(), sweepCompareCheckBox->isChecked());
}

void BatteryFitIntervalWnd::setSweep(QVector<double> aSweepValues, QVector<double> rmse, QVector<double> maxError,
                                     QVector<double> relaxedNo, double referenceRmse, double referenceMaxError,
                                     QVector<unsigned int> cycleIndexes,
                                     QVector<QVector<double>> cycleRmse, QVector<QVector<double>> cycleMaxError,
                                     QVector<double> relaxationTime, QVector<QVector<double>> cycleRelaxationTime)
{
    sweepValues = aSweepValues;
    sweepRmse = rmse;
    sweepMaxError = maxError;
    sweepRelaxedNo = relaxedNo;
    sweepCycleIndexes = cycleIndexes;
    sweepCycleRmse = cycleRmse;
    sweepCycleMaxError = cycleMaxError;
    sweepRelaxationTime = relaxationTime;
    sweepCycleRelaxationTime = cycleRelaxationTime;
    sweepReferenceRmse = referenceRmse;
    sweepReferenceMaxError = referenceMaxError;

    sweepPlotUpdate();

    if(sweepValues.isEmpty()) return;

    sweepSlider->setEnabled(true);
    sweepSlider->setRange(0, sweepValues.size() - 1);
    sweepSlider->setValue(0);
    onSweepSliderMoved(0);
}

void BatteryFitIntervalWnd::sweepPlotUpdate()
{
    int view = sweepViewCombo->currentIndex();

    sweepPlot->clearGraphs();

    if(sweepValues.isEmpty())
    {
        sweepPlot->replot();
        return;
    }

    sweepPlot->yAxis2->setVisible(view == 0);

    if(view == 0)
    {
        QCPGraph *graph = sweepPlot->addGraph();

        graph->setName("RMSE");
        graph->setPen(QPen(QColor(200, 60, 20), 2));
        graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(200, 60, 20), QColor(200, 60, 20), 6));
        graph->setData(sweepValues, sweepRmse, true);

        graph = sweepPlot->addGraph();
        graph->setName("Max error");
        graph->setPen(QPen(QColor(30, 110, 200), 2));
        graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(30, 110, 200), QColor(30, 110, 200), 6));
        graph->setData(sweepValues, sweepMaxError, true);

        /*Relaxation time is seconds, so it gets its own axis on the right*/
        sweepPlot->yAxis2->setLabel("Average time to relaxation [s]");
        sweepPlot->yAxis2->setTickLabels(true);

        graph = sweepPlot->addGraph(sweepPlot->xAxis, sweepPlot->yAxis2);
        graph->setName("Average relaxation time");
        graph->setPen(QPen(QColor(30, 150, 60), 2, Qt::DashDotLine));
        graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, QColor(30, 150, 60), QColor(30, 150, 60), 6));
        graph->setData(sweepValues, sweepRelaxationTime, true);
    }
    else
    {
        const QVector<QVector<double>> &source = (view == 1) ? sweepCycleRmse :
                                                 ((view == 2) ? sweepCycleMaxError : sweepCycleRelaxationTime);

        /*One curve per cycle, so a cycle that behaves differently from the rest
          stands out instead of disappearing into the average*/
        for(int i = 0; i < source.size() && i < sweepCycleIndexes.size(); i++)
        {
            QVector<double> keys;
            QVector<double> values;
            QCPGraph *graph;
            QColor color = QColor::fromHsv((i * 360) / qMax(1, source.size()), 200, 200);

            for(int point = 0; point < source[i].size() && point < sweepValues.size(); point++)
            {
                if(source[i][point] < 0) continue;

                keys.append(sweepValues[point]);
                values.append(source[i][point]);
            }

            if(keys.isEmpty()) continue;

            graph = sweepPlot->addGraph();
            graph->setName("Cycle " + QString::number(sweepCycleIndexes[i]));
            graph->setPen(QPen(color, 1));
            graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, color, color, 4));
            graph->setData(keys, values, true);
        }

        QCPGraph *averageGraph = sweepPlot->addGraph();

        averageGraph->setName("Average");
        averageGraph->setPen(QPen(Qt::black, 2));
        averageGraph->setData(sweepValues, (view == 1) ? sweepRmse :
                                           ((view == 2) ? sweepMaxError : sweepRelaxationTime), true);
    }

    sweepPlot->yAxis->setLabel((view == 3) ? "Time to relaxation [s]" : "Fit error over the whole pause [mV]");

    /*The reference is a fit error, so it says nothing about the relaxation time*/
    if((sweepReferenceRmse > 0) && (view != 3))
    {
        QVector<double> referenceValues;
        QVector<double> referenceLine;
        QCPGraph *graph;

        referenceValues << sweepValues.first() << sweepValues.last();

        if(view != 2)
        {
            referenceLine.clear();
            referenceLine << sweepReferenceRmse << sweepReferenceRmse;
            graph = sweepPlot->addGraph();
            graph->setName("RMSE, fit to pause end");
            graph->setPen(QPen(QColor(200, 60, 20), 1, Qt::DashLine));
            graph->setData(referenceValues, referenceLine, true);
        }

        if(view != 1)
        {
            referenceLine.clear();
            referenceLine << sweepReferenceMaxError << sweepReferenceMaxError;
            graph = sweepPlot->addGraph();
            graph->setName("Max error, fit to pause end");
            graph->setPen(QPen(QColor(30, 110, 200), 1, Qt::DashLine));
            graph->setData(referenceValues, referenceLine, true);
        }
    }

    sweepPlot->rescaleAxes();
    BATTERYPARAMSPLOT_Apply(sweepPlot, plotSettings);
}

void BatteryFitIntervalWnd::onSweepViewChanged()
{
    sweepPlotUpdate();
}

void BatteryFitIntervalWnd::onSweepSliderMoved(int position)
{
    bool thresholdSwept = (sweepModeCombo->currentIndex() == 0);

    if(position < 0 || position >= sweepValues.size()) return;

    sweepMarker->setVisible(true);
    sweepMarker->point1->setCoords(sweepValues[position], 0);
    sweepMarker->point2->setCoords(sweepValues[position], 1);
    sweepPlot->replot();

    sweepPointLabel->setText(QString("%1 %2   |   RMSE %3 mV   |   max %4 mV   |   relaxation %5 s   |   %6 cycles relaxed")
                             .arg(sweepValues[position], 0, 'f', thresholdSwept ? 2 : 1)
                             .arg(thresholdSwept ? "mV" : "s")
                             .arg(sweepRmse[position], 0, 'f', 3)
                             .arg(sweepMaxError[position], 0, 'f', 3)
                             .arg(sweepRelaxationTime.value(position), 0, 'f', 1)
                             .arg((int)sweepRelaxedNo[position]));
}

void BatteryFitIntervalWnd::setData(QVector<batteryfitinterval_cycle_t> aCycles)
{
    cycles = aCycles;

    updatePlots();
    updateTable();
    updateSummary();
}

/*Only the visible series take part in the scaling, so switching one off fills
  the plot with what is left*/
void BatteryFitIntervalWnd::rescale(QCustomPlot *plot, QVector<QCPBars*> &bars, QVector<QCheckBox*> &checkBoxes)
{
    bool first = true;

    plot->xAxis->rescale();

    for(int i = 0; i < bars.size() && i < checkBoxes.size(); i++)
    {
        bars[i]->setVisible(checkBoxes[i]->isChecked());
        bars[i]->removeFromLegend();

        if(!checkBoxes[i]->isChecked()) continue;

        bars[i]->addToLegend();
        bars[i]->rescaleValueAxis(!first);
        first = false;
    }

    if(!first)
    {
        QCPRange range = plot->yAxis->range();

        plot->yAxis->setRange(range.lower - range.size() * 0.1, range.upper + range.size() * 0.1);
        if(plot->yAxis->range().lower > 0) plot->yAxis->setRangeLower(0);
    }

    plot->xAxis->setRange(plot->xAxis->range().lower - 0.5, plot->xAxis->range().upper + 0.5);
    plot->replot();
}

void BatteryFitIntervalWnd::updatePlots()
{
    QVector<double> keys;
    QVector<QVector<double>> errorValues(4);
    QVector<QVector<double>> differenceValues(4);
    QSharedPointer<QCPAxisTickerFixed> errorTicker(new QCPAxisTickerFixed);
    QSharedPointer<QCPAxisTickerFixed> differenceTicker(new QCPAxisTickerFixed);

    for(int i = 0; i < cycles.size(); i++)
    {
        keys.append(cycles[i].index);

        errorValues[0].append(cycles[i].relaxationValid ? cycles[i].relaxationRmse : 0);
        errorValues[1].append(cycles[i].relaxationValid ? cycles[i].relaxationMaxError : 0);
        errorValues[2].append(cycles[i].pauseValid ? cycles[i].pauseRmse : 0);
        errorValues[3].append(cycles[i].pauseValid ? cycles[i].pauseMaxError : 0);

        bool both = cycles[i].relaxationValid && cycles[i].pauseValid;

        differenceValues[0].append(both ? prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationR1, cycles[i].pauseR1) : 0);
        differenceValues[1].append(both ? prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationC1, cycles[i].pauseC1) : 0);
        differenceValues[2].append(both ? prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationR2, cycles[i].pauseR2) : 0);
        differenceValues[3].append(both ? prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationC2, cycles[i].pauseC2) : 0);
    }

    for(int i = 0; i < 4; i++)
    {
        errorBars[i]->setData(keys, errorValues[i], true);
        differenceBars[i]->setData(keys, differenceValues[i], true);
    }

    errorTicker->setTickStep(1.0);
    errorTicker->setScaleStrategy(QCPAxisTickerFixed::ssNone);
    errorPlot->xAxis->setTicker(errorTicker);

    differenceTicker->setTickStep(1.0);
    differenceTicker->setScaleStrategy(QCPAxisTickerFixed::ssNone);
    differencePlot->xAxis->setTicker(differenceTicker);

    rescale(errorPlot, errorBars, errorCheckBoxes);
    rescale(differencePlot, differenceBars, differenceCheckBoxes);

    BATTERYPARAMSPLOT_Apply(errorPlot, plotSettings);
    BATTERYPARAMSPLOT_Apply(differencePlot, plotSettings);
}

void BatteryFitIntervalWnd::updateTable()
{
    table->setRowCount(0);

    for(int i = 0; i < cycles.size(); i++)
    {
        int row = table->rowCount();
        bool both = cycles[i].relaxationValid && cycles[i].pauseValid;

        table->insertRow(row);
        table->setItem(row, 0, prvBATTERYFITINTERVAL_ItemCreate(QString::number(cycles[i].index)));
        table->setItem(row, 1, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].relaxationValid ? QString::number(cycles[i].relaxationR1 * 1000.0, 'f', 2) : "-"));
        table->setItem(row, 2, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].pauseValid ? QString::number(cycles[i].pauseR1 * 1000.0, 'f', 2) : "-"));
        table->setItem(row, 3, prvBATTERYFITINTERVAL_ItemCreate(both ? QString::number(prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationR1, cycles[i].pauseR1), 'f', 1) : "-"));
        table->setItem(row, 4, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].relaxationValid ? QString::number(cycles[i].relaxationC1, 'f', 1) : "-"));
        table->setItem(row, 5, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].pauseValid ? QString::number(cycles[i].pauseC1, 'f', 1) : "-"));
        table->setItem(row, 6, prvBATTERYFITINTERVAL_ItemCreate(both ? QString::number(prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationC1, cycles[i].pauseC1), 'f', 1) : "-"));
        table->setItem(row, 7, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].relaxationValid ? QString::number(cycles[i].relaxationR2 * 1000.0, 'f', 2) : "-"));
        table->setItem(row, 8, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].pauseValid ? QString::number(cycles[i].pauseR2 * 1000.0, 'f', 2) : "-"));
        table->setItem(row, 9, prvBATTERYFITINTERVAL_ItemCreate(both ? QString::number(prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationR2, cycles[i].pauseR2), 'f', 1) : "-"));
        table->setItem(row, 10, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].relaxationValid ? QString::number(cycles[i].relaxationC2, 'f', 1) : "-"));
        table->setItem(row, 11, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].pauseValid ? QString::number(cycles[i].pauseC2, 'f', 1) : "-"));
        table->setItem(row, 12, prvBATTERYFITINTERVAL_ItemCreate(both ? QString::number(prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationC2, cycles[i].pauseC2), 'f', 1) : "-"));
        table->setItem(row, 13, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].relaxationValid ? QString::number(cycles[i].relaxationRmse, 'f', 3) : "-"));
        table->setItem(row, 14, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].pauseValid ? QString::number(cycles[i].pauseRmse, 'f', 3) : "-"));
        table->setItem(row, 15, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].relaxationValid ? QString::number(cycles[i].relaxationMaxError, 'f', 3) : "-"));
        table->setItem(row, 16, prvBATTERYFITINTERVAL_ItemCreate(cycles[i].pauseValid ? QString::number(cycles[i].pauseMaxError, 'f', 3) : "-"));
    }

    table->resizeColumnsToContents();
}

void BatteryFitIntervalWnd::updateSummary()
{
    double relaxationRmseSum = 0;
    double pauseRmseSum = 0;
    double r1DifferenceSum = 0;
    double c1DifferenceSum = 0;
    int validNo = 0;

    for(int i = 0; i < cycles.size(); i++)
    {
        if(!cycles[i].relaxationValid || !cycles[i].pauseValid) continue;

        relaxationRmseSum += cycles[i].relaxationRmse;
        pauseRmseSum += cycles[i].pauseRmse;
        r1DifferenceSum += fabs(prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationR1, cycles[i].pauseR1));
        c1DifferenceSum += fabs(prvBATTERYFITINTERVAL_Difference(cycles[i].relaxationC1, cycles[i].pauseC1));
        validNo++;
    }

    if(validNo == 0)
    {
        summaryLabel->setText("No cycle could be fitted over both intervals");
        return;
    }

    summaryLabel->setText(QString("%1 cycles   |   average RMSE %2 mV to relaxation, %3 mV to pause end   |   "
                                  "average difference R1 %4 %, C1 %5 %")
                          .arg(validNo)
                          .arg(relaxationRmseSum / validNo, 0, 'f', 3)
                          .arg(pauseRmseSum / validNo, 0, 'f', 3)
                          .arg(r1DifferenceSum / validNo, 0, 'f', 1)
                          .arg(c1DifferenceSum / validNo, 0, 'f', 1));
}

void BatteryFitIntervalWnd::onSeriesToggled()
{
    rescale(errorPlot, errorBars, errorCheckBoxes);
    rescale(differencePlot, differenceBars, differenceCheckBoxes);
}

void BatteryFitIntervalWnd::onSaveImage()
{
    QCustomPlot *plot = qobject_cast<QCustomPlot*>(tabWidget->currentWidget());

    if(plot == NULL) plot = (tabWidget->currentIndex() == 1) ? differencePlot : errorPlot;

    BATTERYPARAMSPLOT_Save(plot, this, "fit_interval_comparison");
}
