#include "batterycyclewnd.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <math.h>

#define BATTERYCYCLE_PLOT_MIN_WIDTH     600
#define BATTERYCYCLE_PLOT_MIN_HEIGHT    180
#define BATTERYCYCLE_EDIT_WIDTH         100
#define BATTERYCYCLE_STEP_BUTTON_WIDTH  30
#define BATTERYCYCLE_BUTTON_WIDTH       90
#define BATTERYCYCLE_ROW_HEIGHT         28

BatteryCycleWnd::BatteryCycleWnd(QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *markerLayout = new QHBoxLayout();
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    setFont(defaultFont);
    setWindowTitle("Battery cycle");

    infoLabel = new QLabel(this);
    mainLayout->addWidget(infoLabel);

    modelLabel = new QLabel(this);
    mainLayout->addWidget(modelLabel);

    markerLayout->addLayout(createMarkerRow("Pulse Start", &pulseStartEdit));
    markerLayout->addSpacing(15);
    markerLayout->addLayout(createMarkerRow("Pulse End", &pulseEndEdit));
    markerLayout->addSpacing(15);
    markerLayout->addLayout(createMarkerRow("Pause Start", &pauseStartEdit));
    markerLayout->addSpacing(15);
    markerLayout->addLayout(createMarkerRow("Pause End", &pauseEndEdit));
    markerLayout->addStretch();
    mainLayout->addLayout(markerLayout);

    pulseEndEdit->setToolTip("Pulse End position. Resistance is calculated between this point and Pause Start");
    pauseStartEdit->setToolTip("Pause Start position. Resistance is calculated between Pulse End and this point");

    restoreButton = new QPushButton("Restore", this);
    restoreButton->setToolTip("Return all markers to the positions reported by the device");
    saveButton = new QPushButton("Save", this);
    saveButton->setToolTip("Store adjusted markers and the recalculated resistance in the cycles table");
    restoreButton->setFixedSize(BATTERYCYCLE_BUTTON_WIDTH, BATTERYCYCLE_ROW_HEIGHT);
    saveButton->setFixedSize(BATTERYCYCLE_BUTTON_WIDTH, BATTERYCYCLE_ROW_HEIGHT);

    buttonLayout->addStretch();
    buttonLayout->addWidget(restoreButton);
    buttonLayout->addWidget(saveButton);
    mainLayout->addLayout(buttonLayout);

    voltagePlot = new Plot(BATTERYCYCLE_PLOT_MIN_WIDTH, BATTERYCYCLE_PLOT_MIN_HEIGHT, false, this);
    voltagePlot->setTitle("Voltage");
    voltagePlot->setYLabel("[V]");
    voltagePlot->setXLabel("[ms]");
    voltagePlot->scatterAddGraph();
    mainLayout->addWidget(voltagePlot, 1);

    currentPlot = new Plot(BATTERYCYCLE_PLOT_MIN_WIDTH, BATTERYCYCLE_PLOT_MIN_HEIGHT, false, this);
    currentPlot->setTitle("Current");
    currentPlot->setYLabel("[mA]");
    currentPlot->setXLabel("[ms]");
    currentPlot->scatterAddGraph();
    mainLayout->addWidget(currentPlot, 1);

    connect(restoreButton, &QPushButton::clicked, this, &BatteryCycleWnd::onRestore);
    connect(saveButton, &QPushButton::clicked, this, &BatteryCycleWnd::onSave);

    plotSettings = BATTERYPARAMSPLOT_SettingsDefault();

    resize(900, 650);
}

int BatteryCycleWnd::keyPositionGet(QVector<double> keys, double key)
{
    int position = -1;
    double distance = 0;

    for(int i = 0; i < keys.size(); i++)
    {
        double currentDistance = qAbs(keys[i] - key);

        if((position < 0) || (currentDistance < distance))
        {
            position = i;
            distance = currentDistance;
        }
    }

    return position;
}

void BatteryCycleWnd::addMarker(Plot *plot, QVector<double> values, double key, QString name)
{
    int position = keyPositionGet(cycle.plotKeys, key);

    if(position < 0 || position >= values.size()) return;

    plot->scatterAddDataWithName(values[position], position, name);
}

QHBoxLayout* BatteryCycleWnd::createMarkerRow(QString name, QLineEdit **edit)
{
    QHBoxLayout *rowLayout = new QHBoxLayout();
    QPushButton *stepBack = new QPushButton("<", this);
    QPushButton *stepForward = new QPushButton(">", this);

    *edit = new QLineEdit(this);
    (*edit)->setFixedSize(BATTERYCYCLE_EDIT_WIDTH, BATTERYCYCLE_ROW_HEIGHT);
    stepBack->setFixedSize(BATTERYCYCLE_STEP_BUTTON_WIDTH, BATTERYCYCLE_ROW_HEIGHT);
    stepForward->setFixedSize(BATTERYCYCLE_STEP_BUTTON_WIDTH, BATTERYCYCLE_ROW_HEIGHT);
    stepBack->setToolTip("Move " + name + " one sample back");
    stepForward->setToolTip("Move " + name + " one sample forward");

    /*Step buttons act on the edit box next to them*/
    stepBack->setProperty("markerEdit", QVariant::fromValue((QObject*)(*edit)));
    stepForward->setProperty("markerEdit", QVariant::fromValue((QObject*)(*edit)));

    rowLayout->addWidget(new QLabel(name + " [ms]", this));
    rowLayout->addWidget(stepBack);
    rowLayout->addWidget(*edit);
    rowLayout->addWidget(stepForward);

    connect(stepBack, &QPushButton::clicked, this, &BatteryCycleWnd::onMarkerStepBack);
    connect(stepForward, &QPushButton::clicked, this, &BatteryCycleWnd::onMarkerStepForward);
    connect(*edit, &QLineEdit::editingFinished, this, &BatteryCycleWnd::onMarkersEdited);

    return rowLayout;
}

void BatteryCycleWnd::markerStep(QLineEdit *edit, int step)
{
    int position;

    if(edit == NULL) return;

    position = keyPositionGet(cycle.plotKeys, edit->text().toDouble());

    if(position < 0) return;

    position += step;
    if(position < 0) position = 0;
    if(position >= cycle.plotKeys.size()) position = cycle.plotKeys.size() - 1;

    edit->setText(QString::number(cycle.plotKeys[position], 'f', 3));

    applyMarkers();
}

void BatteryCycleWnd::onMarkerStepBack()
{
    markerStep(qobject_cast<QLineEdit*>(sender()->property("markerEdit").value<QObject*>()), -1);
}

void BatteryCycleWnd::onMarkerStepForward()
{
    markerStep(qobject_cast<QLineEdit*>(sender()->property("markerEdit").value<QObject*>()), 1);
}

void BatteryCycleWnd::onMarkersEdited()
{
    applyMarkers();
}

void BatteryCycleWnd::onRestore()
{
    setCycle(originalCycle);
}

void BatteryCycleWnd::onSave()
{
    emit sigCycleUpdated(cycle);
}

void BatteryCycleWnd::applyMarkers()
{
    int pulseStartPosition = keyPositionGet(cycle.plotKeys, pulseStartEdit->text().toDouble());
    int pulseEndPosition = keyPositionGet(cycle.plotKeys, pulseEndEdit->text().toDouble());
    int pauseStartPosition = keyPositionGet(cycle.plotKeys, pauseStartEdit->text().toDouble());
    int pauseEndPosition = keyPositionGet(cycle.plotKeys, pauseEndEdit->text().toDouble());

    if(pulseStartPosition < 0 || pulseEndPosition < 0) return;
    if(pauseStartPosition < 0 || pauseEndPosition < 0) return;

    cycle.pulseStartKey = cycle.plotKeys[pulseStartPosition];

    cycle.pulseEndKey = cycle.plotKeys[pulseEndPosition];
    cycle.pulseEndVoltage = cycle.plotVoltage[pulseEndPosition];
    cycle.pulseEndCurrent = cycle.plotCurrent[pulseEndPosition];

    cycle.pauseStartKey = cycle.plotKeys[pauseStartPosition];
    cycle.pauseStartVoltage = cycle.plotVoltage[pauseStartPosition];
    cycle.pauseStartCurrent = cycle.plotCurrent[pauseStartPosition];

    cycle.pauseEndKey = cycle.plotKeys[pauseEndPosition];

    cycle.deltaVoltage = cycle.pauseStartVoltage - cycle.pulseEndVoltage;
    cycle.deltaCurrent = cycle.pulseEndCurrent - cycle.pauseStartCurrent;
    cycle.resistanceValid = fabs(cycle.deltaCurrent) > 0.000001;
    cycle.resistance = cycle.resistanceValid ? cycle.deltaVoltage / (cycle.deltaCurrent / 1000.0) : 0;

    pulseStartEdit->setText(QString::number(cycle.pulseStartKey, 'f', 3));
    pulseEndEdit->setText(QString::number(cycle.pulseEndKey, 'f', 3));
    pulseStartEdit->setText(QString::number(cycle.pulseStartKey, 'f', 3));
    pulseEndEdit->setText(QString::number(cycle.pulseEndKey, 'f', 3));
    pauseStartEdit->setText(QString::number(cycle.pauseStartKey, 'f', 3));
    pauseEndEdit->setText(QString::number(cycle.pauseEndKey, 'f', 3));

    updateInfo();
    updatePlots();
}

void BatteryCycleWnd::updateInfo()
{
    bool moved = (cycle.pulseStartKey != originalCycle.pulseStartKey) ||
                 (cycle.pulseEndKey != originalCycle.pulseEndKey) ||
                 (cycle.pauseStartKey != originalCycle.pauseStartKey) ||
                 (cycle.pauseEndKey != originalCycle.pauseEndKey);

    infoLabel->setText("Cycle " + QString::number(cycle.index) +
                       "   |   " + QString::number(cycle.charge, 'f', 3) + " mAh this cycle" +
                       "   |   " + QString::number(cycle.chargeTotal, 'f', 3) + " mAh total" +
                       (cycle.socValid ? "   |   " + QString::number(cycle.socUsed, 'f', 2) + " % SoC used" : "") +
                       "   |   duration " + QString::number(cycle.pauseEndKey - cycle.pulseStartKey, 'f', 1) + " ms" +
                       "   |   dV " + QString::number(cycle.deltaVoltage * 1000.0, 'f', 2) + " mV" +
                       "   |   dI " + QString::number(cycle.deltaCurrent, 'f', 2) + " mA" +
                       "   |   R0 " + (cycle.resistanceValid ? QString::number(cycle.resistance * 1000.0, 'f', 2) + " mOhm" : "-") +
                       (moved ? "   |   markers moved, not saved" : ""));

    if(!cycle.relaxationReached)
    {
        modelLabel->setText("Relaxation not reached, RC model not calculated");
        modelLabel->setStyleSheet("color: rgb(170, 130, 0);");
        return;
    }

    if(!cycle.modelValid)
    {
        modelLabel->setText("Relaxed at " + QString::number(cycle.relaxationKey, 'f', 1) + " ms, RC model could not be fitted");
        modelLabel->setStyleSheet("color: rgb(170, 130, 0);");
        return;
    }

    modelLabel->setStyleSheet("color: rgb(0, 120, 0);");
    modelLabel->setText("Relaxed at " + QString::number(cycle.relaxationKey, 'f', 1) + " ms   |   " +
                        QString(cycle.model == BATTERYPARAMS_MODEL_SECOND_ORDER ? "2nd order" : "1st order") +
                        "   |   R1 " + QString::number(cycle.r1 * 1000.0, 'f', 2) + " mOhm" +
                        "   C1 " + QString::number(cycle.c1, 'f', 1) + " F" +
                        "   tau1 " + QString::number(cycle.tauFast, 'f', 2) + " s" +
                        (cycle.model == BATTERYPARAMS_MODEL_SECOND_ORDER ?
                            "   |   R2 " + QString::number(cycle.r2 * 1000.0, 'f', 2) + " mOhm" +
                            "   C2 " + QString::number(cycle.c2, 'f', 1) + " F" +
                            "   tau2 " + QString::number(cycle.tauSlow, 'f', 2) + " s" : "") +
                        "   |   RMSE " + QString::number(cycle.fitRmse * 1000.0, 'f', 3) + " mV" +
                        "   max " + QString::number(cycle.fitMaxError * 1000.0, 'f', 3) + " mV");

    saveButton->setEnabled(moved);
    restoreButton->setEnabled(moved);
}

void BatteryCycleWnd::setPlotSettings(batteryparams_plot_settings_t settings)
{
    plotSettings = settings;

    voltagePlot->applyStyle(settings.fontFamily, settings.labelFontSize, settings.tickFontSize,
                            settings.lineWidth, settings.gridVisible, settings.minorGridVisible);
    currentPlot->applyStyle(settings.fontFamily, settings.labelFontSize, settings.tickFontSize,
                            settings.lineWidth, settings.gridVisible, settings.minorGridVisible);
}

void BatteryCycleWnd::updatePlots()
{
    voltagePlot->scatterClearMarkers();
    currentPlot->scatterClearMarkers();
    voltagePlot->markersAtKeyClear();
    currentPlot->markersAtKeyClear();
    voltagePlot->overlayClear();

    voltagePlot->setData(cycle.plotVoltage, cycle.plotKeys);
    currentPlot->setData(cycle.plotCurrent, cycle.plotKeys);

    addMarker(voltagePlot, cycle.plotVoltage, cycle.pulseStartKey, "Pulse Start");
    addMarker(voltagePlot, cycle.plotVoltage, cycle.pulseEndKey, "Pulse End");
    addMarker(voltagePlot, cycle.plotVoltage, cycle.pauseStartKey, "Pause Start");
    addMarker(voltagePlot, cycle.plotVoltage, cycle.pauseEndKey, "Pause End");

    addMarker(currentPlot, cycle.plotCurrent, cycle.pulseStartKey, "Pulse Start");
    addMarker(currentPlot, cycle.plotCurrent, cycle.pulseEndKey, "Pulse End");
    addMarker(currentPlot, cycle.plotCurrent, cycle.pauseStartKey, "Pause Start");
    addMarker(currentPlot, cycle.plotCurrent, cycle.pauseEndKey, "Pause End");

    if(cycle.relaxationReached)
    {
        voltagePlot->markerAddAtKey(cycle.relaxationKey, cycle.relaxationVoltage, "Relaxed", QColor(230, 180, 0));
        currentPlot->markerAddAtKey(cycle.relaxationKey, 0, "Relaxed", QColor(230, 180, 0));
    }

    if(cycle.modelValid && !cycle.fitKeys.isEmpty())
    {
        voltagePlot->overlaySetData(cycle.fitVoltage, cycle.fitKeys);
    }

    if(!cycle.plotKeys.isEmpty())
    {
        voltagePlot->zoomToKeyRange(cycle.plotKeys.first(), cycle.plotKeys.last());
        currentPlot->zoomToKeyRange(cycle.plotKeys.first(), cycle.plotKeys.last());
    }

    setPlotSettings(plotSettings);
}

void BatteryCycleWnd::setCycle(batteryparams_cycle_t aCycle)
{
    cycle = aCycle;
    originalCycle = aCycle;

    setWindowTitle("Battery cycle " + QString::number(cycle.index));

    pulseStartEdit->setText(QString::number(cycle.pulseStartKey, 'f', 3));
    pulseEndEdit->setText(QString::number(cycle.pulseEndKey, 'f', 3));
    pauseStartEdit->setText(QString::number(cycle.pauseStartKey, 'f', 3));
    pauseEndEdit->setText(QString::number(cycle.pauseEndKey, 'f', 3));

    updateInfo();
    updatePlots();
}
