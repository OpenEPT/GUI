#include "batterycyclewnd.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <math.h>

#define BATTERYCYCLE_PLOT_MIN_WIDTH     500
#define BATTERYCYCLE_PLOT_MIN_HEIGHT    140
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

    fitSettings = BatteryParamsExtraction::settingsDefault();

    infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);
    infoLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    mainLayout->addWidget(infoLabel);

    modelLabel = new QLabel(this);
    modelLabel->setWordWrap(true);
    modelLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
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

    fitButton = new QPushButton("Fit cycle points", this);
    fitButton->setToolTip("Move Pulse End and Pause Start to the settled samples on both sides of the current step");
    fitButton->setFixedSize(BATTERYCYCLE_BUTTON_WIDTH + 60, BATTERYCYCLE_ROW_HEIGHT);

    restoreButton = new QPushButton("Restore", this);
    restoreButton->setToolTip("Return all markers to the positions reported by the device");
    saveButton = new QPushButton("Save", this);
    saveButton->setToolTip("Store adjusted markers and the recalculated resistance in the cycles table");
    restoreButton->setFixedSize(BATTERYCYCLE_BUTTON_WIDTH, BATTERYCYCLE_ROW_HEIGHT);
    saveButton->setFixedSize(BATTERYCYCLE_BUTTON_WIDTH, BATTERYCYCLE_ROW_HEIGHT);

    buttonLayout->addWidget(fitButton);
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

    /*With both plots locked, panning or zooming the X axis of one follows on the other,
      so the voltage and the current stay aligned in time*/
    connect(voltagePlot, &Plot::sigXRangeChanged, this, [this](QCPRange range){ currentPlot->setXRangeSynced(range); });
    connect(currentPlot, &Plot::sigXRangeChanged, this, [this](QCPRange range){ voltagePlot->setXRangeSynced(range); });

    connect(fitButton, &QPushButton::clicked, this, &BatteryCycleWnd::onFitPoints);
    connect(restoreButton, &QPushButton::clicked, this, &BatteryCycleWnd::onRestore);
    connect(saveButton, &QPushButton::clicked, this, &BatteryCycleWnd::onSave);

    plotSettings = BATTERYPARAMSPLOT_SettingsDefault();

    analysisEmit = false;
    analysisGen = 0;
    analysisRunGen = -1;
    connect(&analysisWatcher, &QFutureWatcher<batteryparams_cycle_t>::finished,
            this, &BatteryCycleWnd::onAnalysisFinished);

    /*A normal top level window with minimize, maximize and close, so it can be resized
      and maximized instead of opening larger than the screen*/
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    int width = 900;
    int height = 650;

    if(QGuiApplication::primaryScreen() != NULL)
    {
        QRect available = QGuiApplication::primaryScreen()->availableGeometry();
        if(width > available.width() - 40) width = available.width() - 40;
        if(height > available.height() - 40) height = available.height() - 40;
    }

    resize(width, height);
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

    QLabel *nameLabel = new QLabel(name + " [ms]", this);

    markerLabels.append(nameLabel);
    markerNames.append(name);

    rowLayout->addWidget(nameLabel);
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

    position = keyPositionGet(cycle.plotKeys, edit->text().toDouble() * timeScale());

    if(position < 0) return;

    position += step;
    if(position < 0) position = 0;
    if(position >= cycle.plotKeys.size()) position = cycle.plotKeys.size() - 1;

    edit->setText(QString::number(cycle.plotKeys[position] / timeScale(), 'f', timeDecimals()));

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

double BatteryCycleWnd::timeScale()
{
    return BATTERYPARAMSPLOT_TimeScale(plotSettings.timeUnit);
}

QString BatteryCycleWnd::timeSuffix()
{
    return BATTERYPARAMSPLOT_TimeSuffix(plotSettings.timeUnit);
}

int BatteryCycleWnd::timeDecimals()
{
    return BATTERYPARAMSPLOT_TimeDecimals(plotSettings.timeUnit);
}

void BatteryCycleWnd::updateMarkerLabels()
{
    for(int i = 0; i < markerLabels.size() && i < markerNames.size(); i++)
    {
        markerLabels[i]->setText(markerNames[i] + " [" + timeSuffix() + "]");
    }
}

void BatteryCycleWnd::refreshMarkerEdits()
{
    pulseStartEdit->setText(QString::number(cycle.pulseStartKey / timeScale(), 'f', timeDecimals()));
    pulseEndEdit->setText(QString::number(cycle.pulseEndKey / timeScale(), 'f', timeDecimals()));
    pauseStartEdit->setText(QString::number(cycle.pauseStartKey / timeScale(), 'f', timeDecimals()));
    pauseEndEdit->setText(QString::number(cycle.pauseEndKey / timeScale(), 'f', timeDecimals()));
}

void BatteryCycleWnd::setSettings(batteryparams_settings_t settings)
{
    fitSettings = settings;
}

void BatteryCycleWnd::onFitPoints()
{
    if(!BatteryParamsExtraction::cyclePointsFit(&cycle)) return;

    cycle.incomplete = false;

    refreshMarkerEdits();

    runAnalysis(true);
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
    int pulseStartPosition = keyPositionGet(cycle.plotKeys, pulseStartEdit->text().toDouble() * timeScale());
    int pulseEndPosition = keyPositionGet(cycle.plotKeys, pulseEndEdit->text().toDouble() * timeScale());
    int pauseStartPosition = keyPositionGet(cycle.plotKeys, pauseStartEdit->text().toDouble() * timeScale());
    int pauseEndPosition = keyPositionGet(cycle.plotKeys, pauseEndEdit->text().toDouble() * timeScale());

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

    refreshMarkerEdits();

    runAnalysis(false);
}

void BatteryCycleWnd::runAnalysis(bool emitWhenDone)
{
    /*The fit runs in a worker thread so moving the points never blocks the window.
      analyzeCycle only reads the sample vectors and writes the fit and average vectors,
      so the large input vectors are shared through copy on write (no deep copy, the
      worker never writes them) and only the output vectors are given fresh empty storage
      so the worker never shares a written container with the one the GUI thread draws.
      If the points are moved again while a fit is running, setFuture drops the old one
      and only the newest result reaches the GUI*/
    batteryparams_cycle_t snapshot = cycle;

    snapshot.fitKeys = QVector<double>();
    snapshot.fitVoltage = QVector<double>();
    snapshot.avgKeys = QVector<double>();
    snapshot.avgVoltage = QVector<double>();

    analysisEmit = emitWhenDone;
    analysisRunGen = ++analysisGen;

    modelLabel->setText("Calculating...");
    modelLabel->setStyleSheet("color: gray;");

    int gen = analysisRunGen;
    batteryparams_settings_t settings = fitSettings;

    analysisWatcher.setFuture(QtConcurrent::run([this, snapshot, settings, gen]() mutable -> batteryparams_cycle_t
    {
        BatteryParamsExtraction::nameCurrentThread(snapshot.index);

        /*Each stage reports its name. The label lives in the GUI thread, so the update is
          posted there, and a stage from a superseded run is ignored by the generation*/
        auto progress = [this, gen](const QString &step)
        {
            QMetaObject::invokeMethod(this, [this, gen, step]()
            {
                if(gen == analysisGen) modelLabel->setText("Calculating: " + step + "...");
            }, Qt::QueuedConnection);
        };

        BatteryParamsExtraction::analyzeCycle(&snapshot, settings, progress);
        return snapshot;
    }));
}

void BatteryCycleWnd::onAnalysisFinished()
{
    if(!analysisWatcher.future().isResultReadyAt(0)) return;

    /*A cycle opened in the meantime invalidates a result still coming from the old one*/
    if(analysisRunGen != analysisGen) return;

    cycle = analysisWatcher.result();

    refreshMarkerEdits();
    updateInfo();
    updatePlots(true);

    if(analysisEmit) emit sigCycleUpdated(cycle);
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
                       "   |   duration " + QString::number((cycle.pauseEndKey - cycle.pulseStartKey) / timeScale(), 'f', 1) + " " + timeSuffix() +
                       "   |   dV " + QString::number(cycle.deltaVoltage * 1000.0, 'f', 2) + " mV" +
                       "   |   dI " + QString::number(cycle.deltaCurrent, 'f', 2) + " mA" +
                       "   |   R0 " + (cycle.resistanceValid ? QString::number(cycle.resistance * 1000.0, 'f', 2) + " mOhm" : "-") +
                       (moved ? "   |   markers moved, not saved" : ""));

    QString reasonSuffix = cycle.modelReason.isEmpty() ? QString() : ("   |   " + cycle.modelReason);

    if(!cycle.relaxationReached && !cycle.modelValid)
    {
        modelLabel->setText("Relaxation not reached, RC model not calculated" + reasonSuffix);
        modelLabel->setStyleSheet("color: rgb(170, 130, 0);");
        return;
    }

    if(!cycle.modelValid)
    {
        modelLabel->setText("Relaxed at " + QString::number(cycle.relaxationKey / timeScale(), 'f', 1) + " " + timeSuffix() + ", RC model could not be fitted" + reasonSuffix);
        modelLabel->setStyleSheet("color: rgb(170, 130, 0);");
        return;
    }

    QString modelPrefix = cycle.relaxationReached
        ? ("Relaxed at " + QString::number(cycle.relaxationKey / timeScale(), 'f', 1) + " " + timeSuffix())
        : "Fitted to pause end (relaxation not reached)";

    QString fastText = cycle.r1Valid
        ? ("   |   R1 " + QString::number(cycle.r1 * 1000.0, 'f', 2) + " mOhm" +
           "   C1 " + QString::number(cycle.c1, 'f', 1) + " F" +
           "   tau1 " + QString::number(cycle.tauFast, 'f', 2) + " s")
        : "   |   R1 / C1 not determined";

    QString slowText = cycle.r2Valid
        ? ("   |   R2 " + QString::number(cycle.r2 * 1000.0, 'f', 2) + " mOhm" +
           "   C2 " + QString::number(cycle.c2, 'f', 1) + " F" +
           "   tau2 " + QString::number(cycle.tauSlow, 'f', 2) + " s")
        : (cycle.model == BATTERYPARAMS_MODEL_SECOND_ORDER ? "   |   R2 / C2 not determined" : "");

    modelLabel->setStyleSheet("color: rgb(0, 120, 0);");
    modelLabel->setText(modelPrefix + "   |   " +
                        QString(cycle.model == BATTERYPARAMS_MODEL_SECOND_ORDER ? "2nd order" : "1st order") +
                        fastText + slowText +
                        "   |   RMSE " + QString::number(cycle.fitRmse * 1000.0, 'f', 3) + " mV" +
                        "   max " + QString::number(cycle.fitMaxError * 1000.0, 'f', 3) + " mV" +
                        reasonSuffix);

    saveButton->setEnabled(moved);
    restoreButton->setEnabled(moved);
}

void BatteryCycleWnd::applyPlotStyle()
{
    voltagePlot->applyStyle(plotSettings.fontFamily, plotSettings.labelFontSize, plotSettings.tickFontSize,
                            plotSettings.lineWidth, plotSettings.gridVisible, plotSettings.minorGridVisible);
    currentPlot->applyStyle(plotSettings.fontFamily, plotSettings.labelFontSize, plotSettings.tickFontSize,
                            plotSettings.lineWidth, plotSettings.gridVisible, plotSettings.minorGridVisible);
}

void BatteryCycleWnd::setPlotSettings(batteryparams_plot_settings_t settings)
{
    bool unitChanged = (settings.timeUnit != plotSettings.timeUnit);

    plotSettings = settings;

    applyPlotStyle();

    if(unitChanged)
    {
        updateMarkerLabels();
        refreshMarkerEdits();
        updateInfo();
        updatePlots();
    }
}

void BatteryCycleWnd::updatePlots(bool keepView)
{
    /*When the view is kept, the current zoom is captured before the data is replaced and
      restored afterwards, so a recalculation does not snap the plot back to the whole
      cycle when the user has zoomed into a part of it*/
    QCPRange keepVoltageX, keepVoltageY, keepCurrentX, keepCurrentY;

    if(keepView)
    {
        keepVoltageX = voltagePlot->plotXRange();
        keepVoltageY = voltagePlot->plotYRange();
        keepCurrentX = currentPlot->plotXRange();
        keepCurrentY = currentPlot->plotYRange();
    }

    voltagePlot->scatterClearMarkers();
    currentPlot->scatterClearMarkers();
    voltagePlot->markersAtKeyClear();
    currentPlot->markersAtKeyClear();
    voltagePlot->overlayClear();
    voltagePlot->shadeClear();
    voltagePlot->clearAverageData();

    QVector<double> scaledKeys;

    for(int i = 0; i < cycle.plotKeys.size(); i++)
    {
        scaledKeys.append(cycle.plotKeys[i] / timeScale());
    }

    voltagePlot->setXLabel("[" + timeSuffix() + "]");
    currentPlot->setXLabel("[" + timeSuffix() + "]");

    voltagePlot->setData(cycle.plotVoltage, scaledKeys, !keepView);
    currentPlot->setData(cycle.plotCurrent, scaledKeys, !keepView);

    /*Averaged pause voltage the RC fit actually used, shown so the effect of the fit
      average window is visible. With a window of 1 it sits on top of the raw samples*/
    if(!cycle.avgVoltage.isEmpty() && (cycle.avgVoltage.size() == cycle.avgKeys.size()))
    {
        QVector<double> scaledAvgKeys;
        for(int i = 0; i < cycle.avgKeys.size(); i++) scaledAvgKeys.append(cycle.avgKeys[i] / timeScale());

        voltagePlot->averageAddGraph();
        voltagePlot->appendAverageData(cycle.avgVoltage, scaledAvgKeys);
    }

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
        voltagePlot->markerAddAtKey(cycle.relaxationKey / timeScale(), cycle.relaxationVoltage, "Relaxed", QColor(230, 180, 0));
        currentPlot->markerAddAtKey(cycle.relaxationKey / timeScale(), 0, "Relaxed", QColor(230, 180, 0));
    }

    if(cycle.modelValid && !cycle.fitKeys.isEmpty())
    {
        QVector<double> scaledFitKeys;

        for(int i = 0; i < cycle.fitKeys.size(); i++)
        {
            scaledFitKeys.append(cycle.fitKeys[i] / timeScale());
        }

        voltagePlot->overlaySetData(cycle.fitVoltage, scaledFitKeys);

        /*Shade the portion of the relaxation each time constant was peeled from:
          the early part feeds the fast branch, the later part the slow branch*/
        if((cycle.relaxationReached || cycle.modelValid) && (cycle.fastPortionEndKey > cycle.pauseStartKey))
        {
            double fastStart = cycle.pauseStartKey / timeScale();
            double fastEnd = cycle.fastPortionEndKey / timeScale();
            double slowEnd = (cycle.fitEndKey > 0 ? cycle.fitEndKey : cycle.relaxationKey) / timeScale();

            voltagePlot->shadeKeyRange(fastStart, fastEnd, QColor(80, 140, 255, 45));

            if((cycle.model == BATTERYPARAMS_MODEL_SECOND_ORDER) && (slowEnd > fastEnd))
            {
                voltagePlot->shadeKeyRange(fastEnd, slowEnd, QColor(255, 160, 60, 40));
            }
        }
    }

    if(!cycle.plotKeys.isEmpty())
    {
        if(keepView)
        {
            voltagePlot->setXRange(keepVoltageX.lower, keepVoltageX.upper);
            voltagePlot->setYRange(keepVoltageY.lower, keepVoltageY.upper);
            currentPlot->setXRange(keepCurrentX.lower, keepCurrentX.upper);
            currentPlot->setYRange(keepCurrentY.lower, keepCurrentY.upper);
        }
        else
        {
            voltagePlot->zoomToKeyRange(scaledKeys.first(), scaledKeys.last());
            currentPlot->zoomToKeyRange(scaledKeys.first(), scaledKeys.last());
        }
    }

    applyPlotStyle();
}

void BatteryCycleWnd::setCycle(batteryparams_cycle_t aCycle)
{
    /*Invalidate a fit still running for the previous cycle so its result is dropped*/
    analysisGen++;

    cycle = aCycle;
    originalCycle = aCycle;

    setWindowTitle("Battery cycle " + QString::number(cycle.index));

    refreshMarkerEdits();

    updateInfo();
    updatePlots();
}
