#include "batteryparamswnd.h"

#include <QApplication>
#include <QPushButton>
#include <QCoreApplication>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QFileDialog>
#include <QProgressDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextStream>
#include <QVBoxLayout>

#define BATTERYPARAMS_COLUMN_NO             21
#define BATTERYPARAMS_SETTINGS_EDIT_WIDTH   80
#define BATTERYPARAMS_SETTINGS_ROW_HEIGHT   28
#define BATTERYPARAMS_AUTO_RESIZE_ROW_NO    10
#define BATTERYPARAMS_FIT_WINDOWS_MAX       5

BatteryParamsWnd::BatteryParamsWnd(QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QStringList header;
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *topLayout = new QHBoxLayout();
    QHBoxLayout *bottomLayout = new QHBoxLayout();

    setFont(defaultFont);
    setWindowTitle("Battery parameters");

    statusLabel = new QLabel("Waiting for the first cycle...", this);
    topLayout->addWidget(statusLabel);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    QHBoxLayout *settingsLayout = new QHBoxLayout();

    settings = BatteryParamsExtraction::settingsDefault();
    parallelAnswer = -1;
    settingsDlg = NULL;
    resistanceTrendWnd = NULL;
    capacitanceTrendWnd = NULL;
    ocvTrendWnd = NULL;
    ocvAnalysisWnd = NULL;
    fitQualityWnd = NULL;
    relaxationTimeWnd = NULL;
    fitIntervalWnd = NULL;
    connectedToFitIntervalWnd = false;
    plotSettings = BATTERYPARAMSPLOT_SettingsDefault();

    settingsButton = new QPushButton("Settings", this);
    settingsButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    settingsButton->setToolTip("Battery capacity, initial state of charge, relaxation and extraction settings");

    resistanceTrendButton = new QPushButton("R vs SoC", this);
    resistanceTrendButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    resistanceTrendButton->setToolTip("Show R0, R1 and R2 against the state of charge");

    capacitanceTrendButton = new QPushButton("C vs SoC", this);
    capacitanceTrendButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    capacitanceTrendButton->setToolTip("Show C1 and C2 against the state of charge");

    ocvTrendButton = new QPushButton("OCV vs SoC", this);
    ocvTrendButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    ocvTrendButton->setToolTip("Show the open circuit voltage against the state of charge");

    ocvAnalysisButton = new QPushButton("OCV analysis", this);
    ocvAnalysisButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    ocvAnalysisButton->setToolTip("State of charge against the open circuit voltage, its sensitivity and the effect of measurement noise");

    fitQualityButton = new QPushButton("Estimation quality", this);
    fitQualityButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    fitQualityButton->setToolTip("Compare how well the first and the second order model describe every cycle");

    relaxationTimeButton = new QPushButton("Relaxation time", this);
    relaxationTimeButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    relaxationTimeButton->setToolTip("Time every cycle needed from the beginning of the pause until the battery relaxed");

    fitIntervalButton = new QPushButton("Fit interval", this);
    fitIntervalButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    fitIntervalButton->setToolTip("Compare the parameters and the errors obtained by fitting up to the relaxation point and up to the end of the pause");

    fitPointsButton = new QPushButton("Fit cycle points", this);
    fitPointsButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    fitPointsButton->setToolTip("Check every cycle and move Pulse End and Pause Start onto the settled levels around the current step.\nCycles that had to be corrected are opened for review");

    fitPointsSaveButton = new QPushButton("Save new points", this);
    fitPointsSaveButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    fitPointsSaveButton->setToolTip("Keep the fitted points on every corrected cycle and clear the marks");

    fitPointsRestoreButton = new QPushButton("Restore old points", this);
    fitPointsRestoreButton->setFixedHeight(BATTERYPARAMS_SETTINGS_ROW_HEIGHT);
    fitPointsRestoreButton->setToolTip("Return every corrected cycle to the points reported by the device");

    settingsLabel = new QLabel(this);
    settingsLabel->setStyleSheet("color: gray;");

    settingsLayout->addWidget(settingsButton);
    settingsLayout->addSpacing(15);
    settingsLayout->addWidget(resistanceTrendButton);
    settingsLayout->addWidget(capacitanceTrendButton);
    settingsLayout->addWidget(ocvTrendButton);
    settingsLayout->addWidget(ocvAnalysisButton);
    settingsLayout->addWidget(fitQualityButton);
    settingsLayout->addWidget(relaxationTimeButton);
    settingsLayout->addWidget(fitIntervalButton);
    settingsLayout->addSpacing(15);
    settingsLayout->addWidget(fitPointsButton);
    settingsLayout->addWidget(fitPointsSaveButton);
    settingsLayout->addWidget(fitPointsRestoreButton);
    settingsLayout->addSpacing(15);
    settingsLayout->addWidget(settingsLabel, 1);
    mainLayout->addLayout(settingsLayout);

    connect(settingsButton, &QPushButton::clicked, this, &BatteryParamsWnd::onSettings);
    connect(resistanceTrendButton, &QPushButton::clicked, this, &BatteryParamsWnd::onResistanceTrend);
    connect(capacitanceTrendButton, &QPushButton::clicked, this, &BatteryParamsWnd::onCapacitanceTrend);
    connect(ocvTrendButton, &QPushButton::clicked, this, &BatteryParamsWnd::onOcvTrend);
    connect(ocvAnalysisButton, &QPushButton::clicked, this, &BatteryParamsWnd::onOcvAnalysis);
    connect(fitQualityButton, &QPushButton::clicked, this, &BatteryParamsWnd::onFitQuality);
    connect(relaxationTimeButton, &QPushButton::clicked, this, &BatteryParamsWnd::onRelaxationTime);
    connect(fitIntervalButton, &QPushButton::clicked, this, &BatteryParamsWnd::onFitInterval);
    connect(fitPointsButton, &QPushButton::clicked, this, &BatteryParamsWnd::onFitCyclePoints);
    connect(fitPointsSaveButton, &QPushButton::clicked, this, &BatteryParamsWnd::onFitPointsSave);
    connect(fitPointsRestoreButton, &QPushButton::clicked, this, &BatteryParamsWnd::onFitPointsRestore);

    updateSettingsLabel();
    updateFitPointsButtons();

    header << "Cycle" << "Start [ms]" << "End [ms]"
           << "Q [mAh]" << "Q total [mAh]" << "SoC used [%]" << "SoC [%]" << "OCV [V]"
           << "V pulse end [V]" << "V pause start [V]" << "dV [mV]"
           << "I pulse end [mA]" << "I pause start [mA]" << "dI [mA]"
           << "R0 [mOhm]"
           << "R1 [mOhm]" << "C1 [F]" << "R2 [mOhm]" << "C2 [F]"
           << "RMSE [mV]" << "Max err [mV]";

    cyclesTable = new QTableWidget(0, BATTERYPARAMS_COLUMN_NO, this);
    cyclesTable->setHorizontalHeaderLabels(header);
    cyclesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    cyclesTable->horizontalHeader()->setSectionsMovable(true);
    cyclesTable->horizontalHeader()->setStretchLastSection(true);
    cyclesTable->horizontalHeader()->setMinimumSectionSize(40);
    cyclesTable->resizeColumnsToContents();
    cyclesTable->verticalHeader()->setVisible(false);
    cyclesTable->verticalHeader()->setDefaultSectionSize(22);
    cyclesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    cyclesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    cyclesTable->setMinimumHeight(200);
    mainLayout->addWidget(cyclesTable, 1);

    summaryLabel = new QLabel(this);
    summaryLabel->setStyleSheet("color: gray;");
    bottomLayout->addWidget(summaryLabel, 1);

    exportButton = new QPushButton("Export CSV", this);
    clearButton = new QPushButton("Clear", this);
    bottomLayout->addWidget(exportButton);
    bottomLayout->addWidget(clearButton);
    mainLayout->addLayout(bottomLayout);

    cyclesTable->setToolTip("Click a cycle to show its waveform.\nColumns can be resized and reordered by dragging the header");

    cycleWnd = NULL;
    fittedPositions.clear();
    fittedOriginals.clear();
    connectedToCycleWnd = false;

    connect(cyclesTable, &QTableWidget::cellClicked, this, &BatteryParamsWnd::onCycleSelected);
    connect(exportButton, &QPushButton::clicked, this, &BatteryParamsWnd::onExport);
    connect(clearButton, &QPushButton::clicked, this, &BatteryParamsWnd::onClear);

    resize(1000, 400);
    updateSummary();
}

void BatteryParamsWnd::setProfileName(QString name)
{
    profileName = name;
    setWindowTitle(name.isEmpty() ? "Battery parameters" : "Battery parameters - " + name);
}

QTableWidgetItem* BatteryParamsWnd::createItem(QString text)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

void BatteryParamsWnd::fillCycleRow(int row, batteryparams_cycle_t cycle)
{
    cyclesTable->setItem(row, 0, createItem(QString::number(cycle.index) + (fittedPositions.contains(row) ? " *" : "")));
    cyclesTable->setItem(row, 1, createItem(QString::number(cycle.pulseStartKey, 'f', 3)));
    cyclesTable->setItem(row, 2, createItem(QString::number(cycle.pauseEndKey, 'f', 3)));
    cyclesTable->setItem(row, 3, createItem(QString::number(cycle.charge, 'f', 3)));
    cyclesTable->setItem(row, 4, createItem(QString::number(cycle.chargeTotal, 'f', 3)));
    cyclesTable->setItem(row, 5, createItem(cycle.socValid ? QString::number(cycle.socUsed, 'f', 2) : "-"));
    cyclesTable->setItem(row, 6, createItem(cycle.socValid ? QString::number(cycle.soc, 'f', 2) : "-"));
    cyclesTable->setItem(row, 7, createItem(cycle.relaxationReached ? QString::number(cycle.ocv, 'f', 4) : "-"));
    cyclesTable->setItem(row, 8, createItem(QString::number(cycle.pulseEndVoltage, 'f', 4)));
    cyclesTable->setItem(row, 9, createItem(QString::number(cycle.pauseStartVoltage, 'f', 4)));
    cyclesTable->setItem(row, 10, createItem(QString::number(cycle.deltaVoltage * 1000.0, 'f', 2)));
    cyclesTable->setItem(row, 11, createItem(QString::number(cycle.pulseEndCurrent, 'f', 2)));
    cyclesTable->setItem(row, 12, createItem(QString::number(cycle.pauseStartCurrent, 'f', 2)));
    cyclesTable->setItem(row, 13, createItem(QString::number(cycle.deltaCurrent, 'f', 2)));
    cyclesTable->setItem(row, 14, createItem(cycle.resistanceValid ? QString::number(cycle.resistance * 1000.0, 'f', 2) : "-"));
    cyclesTable->setItem(row, 15, createItem(cycle.modelValid ? QString::number(cycle.r1 * 1000.0, 'f', 2) : "-"));
    cyclesTable->setItem(row, 16, createItem(cycle.modelValid ? QString::number(cycle.c1, 'f', 1) : "-"));
    cyclesTable->setItem(row, 17, createItem((cycle.modelValid && (cycle.model == BATTERYPARAMS_MODEL_SECOND_ORDER)) ? QString::number(cycle.r2 * 1000.0, 'f', 2) : "-"));
    cyclesTable->setItem(row, 18, createItem((cycle.modelValid && (cycle.model == BATTERYPARAMS_MODEL_SECOND_ORDER)) ? QString::number(cycle.c2, 'f', 1) : "-"));
    cyclesTable->setItem(row, 19, createItem(cycle.modelValid ? QString::number(cycle.fitRmse * 1000.0, 'f', 3) : "-"));
    cyclesTable->setItem(row, 20, createItem(cycle.modelValid ? QString::number(cycle.fitMaxError * 1000.0, 'f', 3) : "-"));

    /*Green marks a cycle where the cell relaxed and the model could be fitted,
      yellow one where it did not*/
    QColor rowColor = cycle.relaxationReached ? QColor(200, 240, 200) : QColor(255, 245, 190);

    for(int column = 0; column < BATTERYPARAMS_COLUMN_NO; column++)
    {
        if(cyclesTable->item(row, column) == NULL) continue;
        cyclesTable->item(row, column)->setBackground(rowColor);
    }
}

void BatteryParamsWnd::addCycleRow(batteryparams_cycle_t cycle)
{
    int row = cyclesTable->rowCount();

    cyclesTable->insertRow(row);
    fillCycleRow(row, cycle);

    /*Column widths follow the content while the first cycles arrive, after that
      they stay as the user left them*/
    if(row < BATTERYPARAMS_AUTO_RESIZE_ROW_NO)
    {
        cyclesTable->resizeColumnsToContents();
    }
    cyclesTable->scrollToBottom();
}

void BatteryParamsWnd::updateSummary()
{
    double sum = 0;
    int validNo = 0;

    for(int i = 0; i < cycles.size(); i++)
    {
        if(!cycles[i].resistanceValid) continue;
        sum += cycles[i].resistance;
        validNo++;
    }

    if(validNo == 0)
    {
        summaryLabel->setText(QString::number(cycles.size()) + " cycles, no resistance calculated yet");
        return;
    }

    summaryLabel->setText(QString::number(cycles.size()) + " cycles, " +
                          QString::number(cycles.last().chargeTotal, 'f', 3) + " mAh extracted" +
                          (cycles.last().socValid ? ", " + QString::number(cycles.last().socUsed, 'f', 2) + " % SoC used" : "") +
                          ", average resistance " + QString::number(sum / validNo * 1000.0, 'f', 2) + " mOhm");
}

/*Used by the offline analysis, where the cycles are detected first and fitted
  afterwards, optionally on all cores*/
void BatteryParamsWnd::addCycles(QVector<batteryparams_cycle_t> newCycles, QProgressDialog *progressDialog)
{
    analyzeCycles(&newCycles, progressDialog, parallelGet(newCycles.size()));

    for(int i = 0; i < newCycles.size(); i++)
    {
        onCycleFinished(newCycles[i]);
    }
}

void BatteryParamsWnd::onCycleStarted(unsigned int index, double key)
{
    statusLabel->setText("Cycle " + QString::number(index) + " ongoing (started at " +
                         QString::number(key, 'f', 3) + " ms)");
}

void BatteryParamsWnd::onCycleFinished(batteryparams_cycle_t cycle)
{
    cycles.append(cycle);
    addCycleRow(cycle);
    updateSummary();
    statusLabel->setText("Cycle " + QString::number(cycle.index) + " done, waiting for the next one...");
}

void BatteryParamsWnd::onCycleSelected(int row, int column)
{
    Q_UNUSED(column);

    if(row < 0 || row >= cycles.size()) return;

    if(cycleWnd == NULL)
    {
        cycleWnd = new BatteryCycleWnd();
        cycleWnd->setAttribute(Qt::WA_QuitOnClose, false);
    }

    if(!connectedToCycleWnd)
    {
        connect(cycleWnd, &BatteryCycleWnd::sigCycleUpdated, this, &BatteryParamsWnd::onCycleUpdated);
        connectedToCycleWnd = true;
    }

    cycleWnd->setPlotSettings(plotSettings);
    cycleWnd->setCycle(cycles[row]);
    cycleWnd->show();
    cycleWnd->raise();
    cycleWnd->activateWindow();
}

void BatteryParamsWnd::updateFitPointsButtons()
{
    fitPointsSaveButton->setEnabled(!fittedPositions.isEmpty());
    fitPointsRestoreButton->setEnabled(!fittedPositions.isEmpty());
}

void BatteryParamsWnd::onFitCyclePoints()
{
    QVector<batteryparams_cycle_t> working = cycles;
    QVector<int> changed;
    QProgressDialog progressDialog("Fitting cycle points...", QString(), 0, cycles.size(), this);
    batteryparams_settings_t usedSettings = settings;
    int cycleNo = cycles.size();
    int fittedNo = 0;

    if(cycles.isEmpty()) return;

    changed.fill(0, cycleNo);

    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setWindowTitle("Fit cycle points");
    progressDialog.setMinimumDuration(0);
    progressDialog.setAutoClose(false);
    progressDialog.setAutoReset(false);
    progressDialog.setValue(0);

    if(!parallelGet(cycleNo))
    {
        for(int i = 0; i < cycleNo; i++)
        {
            progressDialog.setLabelText(QString("Fitting cycle %1 of %2...").arg(i + 1).arg(cycleNo));
            progressDialog.setValue(i);
            QApplication::processEvents();

            if(!BatteryParamsExtraction::cyclePointsFit(&working[i])) continue;

            BatteryParamsExtraction::analyzeCycle(&working[i], usedSettings);
            changed[i] = 1;
        }
    }
    else
    {
        QVector<int> positions;
        QFutureWatcher<void> watcher;

        /*Containers are detached here, in this thread, so that the tasks only write
          through plain pointers and never share a Qt container between threads*/
        batteryparams_cycle_t *workingData = working.data();
        int *changedData = changed.data();

        for(int i = 0; i < cycleNo; i++) positions.append(i);

        /*Every cycle is fitted and analysed on its own entry of the working copy, so
          the tasks never touch the same data*/
        QFuture<void> future = QtConcurrent::map(positions, [workingData, changedData, usedSettings](int position)
        {
            if(!BatteryParamsExtraction::cyclePointsFit(&workingData[position])) return;

            BatteryParamsExtraction::analyzeCycle(&workingData[position], usedSettings);
            changedData[position] = 1;
        });

        watcher.setFuture(future);

        while(!watcher.isFinished())
        {
            progressDialog.setLabelText(QString("Fitting %1 cycles on %2 cores, %3 done...")
                                        .arg(cycleNo)
                                        .arg(QThread::idealThreadCount())
                                        .arg(watcher.progressValue()));
            progressDialog.setValue(watcher.progressValue());
            QApplication::processEvents();
            QThread::msleep(20);
        }
    }

    progressDialog.setValue(cycleNo);
    progressDialog.close();

    for(int i = 0; i < cycleNo; i++)
    {
        if(changed[i] == 0) continue;

        /*Points reported by the device are kept until the correction is confirmed*/
        if(!fittedPositions.contains(i))
        {
            fittedPositions.append(i);
            fittedOriginals.append(cycles[i]);
        }

        cycles[i] = working[i];
        fillCycleRow(i, cycles[i]);
        fittedNo++;
    }

    updateSummary();
    updateFitPointsButtons();

    if(fittedNo == 0)
    {
        statusLabel->setText("Cycle points checked, every cycle already sits on the settled levels");
        return;
    }

    statusLabel->setText("Cycle points fitted on " + QString::number(fittedNo) + " of " +
                         QString::number(cycleNo) + " cycles, marked with * and not confirmed yet");
}

void BatteryParamsWnd::onFitPointsSave()
{
    int no = fittedPositions.size();

    if(no == 0) return;

    fittedPositions.clear();
    fittedOriginals.clear();

    for(int i = 0; i < cycles.size(); i++)
    {
        fillCycleRow(i, cycles[i]);
    }

    updateFitPointsButtons();
    statusLabel->setText("Fitted points kept on " + QString::number(no) + " cycles");
}

void BatteryParamsWnd::onFitPointsRestore()
{
    int no = fittedPositions.size();

    if(no == 0) return;

    for(int i = 0; i < fittedPositions.size() && i < fittedOriginals.size(); i++)
    {
        int position = fittedPositions[i];

        if(position < 0 || position >= cycles.size()) continue;

        cycles[position] = fittedOriginals[i];
    }

    fittedPositions.clear();
    fittedOriginals.clear();

    for(int i = 0; i < cycles.size(); i++)
    {
        fillCycleRow(i, cycles[i]);
    }

    updateSummary();
    updateFitPointsButtons();
    statusLabel->setText("Points reported by the device restored on " + QString::number(no) + " cycles");
}

void BatteryParamsWnd::onCycleUpdated(batteryparams_cycle_t cycle)
{
    for(int i = 0; i < cycles.size(); i++)
    {
        if(cycles[i].index != cycle.index) continue;

        cycles[i] = cycle;
        fillCycleRow(i, cycle);
        updateSummary();
        statusLabel->setText("Cycle " + QString::number(cycle.index) + " markers adjusted manually");
        return;
    }
}

void BatteryParamsWnd::onClear()
{
    fittedPositions.clear();
    fittedOriginals.clear();
    updateFitPointsButtons();

    cycles.clear();
    cyclesTable->setRowCount(0);
    statusLabel->setText("Waiting for the first cycle...");
    updateSummary();
}

void BatteryParamsWnd::setSettings(batteryparams_settings_t aSettings)
{
    settings = aSettings;
    updateSettingsLabel();
    if(settingsDlg != NULL) settingsDlg->setSettings(settings);
}

void BatteryParamsWnd::updateSettingsLabel()
{
    settingsLabel->setText(QString(settings.model == BATTERYPARAMS_MODEL_SECOND_ORDER ? "2nd order" : "1st order") +
                           "   |   relaxation " + QString::number(settings.relaxationThreshold) + " mV / " +
                           QString::number(settings.relaxationWindow) + " s" +
                           "   |   capacity " + (settings.capacity > 0 ? QString::number(settings.capacity) + " mAh" : "unknown") +
                           "   |   initial SoC " + QString::number(settings.initialSoc) + " %" +
                           "   |   fit to " + QString(settings.fitEnd == BATTERYPARAMS_FIT_END_RELAXATION ?
                                                      "relaxation point" : "pause end"));
}

void BatteryParamsWnd::onSettings()
{
    if(settingsDlg == NULL)
    {
        settingsDlg = new BatteryParamsSettingsDlg(this);
    }

    settingsDlg->setSettings(settings);
    settingsDlg->setPlotSettings(plotSettings);

    if(settingsDlg->exec() != QDialog::Accepted) return;

    settings = settingsDlg->getSettings();
    plotSettings = settingsDlg->getPlotSettings();

    if(resistanceTrendWnd != NULL) resistanceTrendWnd->setPlotSettings(plotSettings);
    if(capacitanceTrendWnd != NULL) capacitanceTrendWnd->setPlotSettings(plotSettings);
    if(ocvTrendWnd != NULL) ocvTrendWnd->setPlotSettings(plotSettings);
    if(relaxationTimeWnd != NULL) relaxationTimeWnd->setPlotSettings(plotSettings);
    if(ocvAnalysisWnd != NULL) ocvAnalysisWnd->setPlotSettings(plotSettings);
    if(fitQualityWnd != NULL) fitQualityWnd->setPlotSettings(plotSettings);
    if(fitIntervalWnd != NULL) fitIntervalWnd->setPlotSettings(plotSettings);
    if(cycleWnd != NULL) cycleWnd->setPlotSettings(plotSettings);

    updateSettingsLabel();
    recalculate();

    emit sigSettingsChanged(settings);
}

/*Fitting one cycle does not depend on any other cycle, so the work can be spread
  over all cores. The question is asked once and the answer kept for the rest of
  the session*/
bool BatteryParamsWnd::parallelGet(int cycleNo)
{
    int coreNo = QThread::idealThreadCount();

    if(cycleNo < 2) return false;
    if(coreNo < 2) return false;
    if(parallelAnswer >= 0) return (parallelAnswer == 1);

    parallelAnswer = (QMessageBox::question(this, "Battery parameters",
                                            "Analysis of " + QString::number(cycleNo) + " cycles is about to start.\n\n"
                                            "Use all " + QString::number(coreNo) + " processor cores?\n"
                                            "The analysis finishes faster, but the machine is busy while it runs.",
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::Yes) == QMessageBox::Yes) ? 1 : 0;

    return (parallelAnswer == 1);
}

void BatteryParamsWnd::analyzeCycles(QVector<batteryparams_cycle_t> *target, QProgressDialog *progressDialog, bool parallel)
{
    analyzeCycles(target, progressDialog, parallel, settings);
}

void BatteryParamsWnd::analyzeCycles(QVector<batteryparams_cycle_t> *target, QProgressDialog *progressDialog, bool parallel, batteryparams_settings_t usedSettings)
{
    int cycleNo = target->size();

    if(!parallel)
    {
        for(int i = 0; i < cycleNo; i++)
        {
            if(progressDialog != NULL)
            {
                progressDialog->setLabelText(QString("%1 cycles detected\nProcessing cycle %2 of %1...")
                                             .arg(cycleNo).arg(i + 1));
                progressDialog->setValue(i);
                QApplication::processEvents();
            }

            BatteryParamsExtraction::analyzeCycle(&(*target)[i], usedSettings);
        }

        if(progressDialog != NULL) progressDialog->setValue(cycleNo);
        return;
    }

    QFutureWatcher<void> watcher;
    QFuture<void> future = QtConcurrent::map(*target, [usedSettings](batteryparams_cycle_t &cycle)
    {
        BatteryParamsExtraction::analyzeCycle(&cycle, usedSettings);
    });

    watcher.setFuture(future);

    while(!watcher.isFinished())
    {
        if(progressDialog != NULL)
        {
            progressDialog->setLabelText(QString("%1 cycles detected\nProcessing on %2 cores, %3 of %1 done...")
                                         .arg(cycleNo)
                                         .arg(QThread::idealThreadCount())
                                         .arg(watcher.progressValue()));
            progressDialog->setValue(watcher.progressValue());
        }
        QApplication::processEvents();
        QThread::msleep(20);
    }

    if(progressDialog != NULL) progressDialog->setValue(cycleNo);
}

void BatteryParamsWnd::recalculate()
{
    /*Already collected cycles keep their samples, so changed settings are
      applied to them as well instead of only to the cycles still to come.
      Fitting takes a moment per cycle, so the progress is shown*/
    if(cycles.isEmpty()) return;

    QProgressDialog progressDialog(QString("%1 cycles detected\nRecalculating battery parameters...").arg(cycles.size()),
                                   QString(), 0, cycles.size(), this);

    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(0);
    progressDialog.setAutoClose(false);
    progressDialog.setAutoReset(false);
    progressDialog.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progressDialog.setWindowTitle("Battery parameters");
    progressDialog.setMinimumWidth(320);
    progressDialog.setValue(0);
    QApplication::processEvents();

    for(int i = 0; i < cycles.size(); i++)
    {
        if(settings.capacity <= 0) continue;

        cycles[i].socUsed = cycles[i].chargeTotal / settings.capacity * 100.0;
        cycles[i].soc = settings.initialSoc - cycles[i].socUsed;
        cycles[i].socValid = true;
    }

    analyzeCycles(&cycles, &progressDialog, parallelGet(cycles.size()));

    for(int i = 0; i < cycles.size(); i++)
    {
        fillCycleRow(i, cycles[i]);
    }

    progressDialog.close();

    updateSummary();
}

batteryparams_trend_t BatteryParamsWnd::trendCreate(QString name, QColor color)
{
    batteryparams_trend_t trend;

    trend.name = name;
    trend.color = color;

    return trend;
}

void BatteryParamsWnd::trendShow(BatteryParamsTrendWnd **window, QString title, QString yLabel, QVector<batteryparams_trend_t> trends)
{
    bool empty = true;

    for(int i = 0; i < trends.size(); i++)
    {
        if(trends[i].soc.isEmpty()) continue;
        empty = false;
        break;
    }

    if(empty)
    {
        QMessageBox::warning(this, title, "No relaxed cycle with a known state of charge yet.\n"
                                          "Enter the battery capacity in Settings and wait for a cycle where the battery relaxes.");
        return;
    }

    if(*window == NULL)
    {
        *window = new BatteryParamsTrendWnd();
        (*window)->setAttribute(Qt::WA_QuitOnClose, false);
    }

    (*window)->setPlotSettings(plotSettings);
    (*window)->setTrends(title, yLabel, trends);
    (*window)->show();
    (*window)->raise();
    (*window)->activateWindow();
}

void BatteryParamsWnd::onResistanceTrend()
{
    QVector<batteryparams_trend_t> trends;
    batteryparams_trend_t r0 = trendCreate("R0", QColor(200, 60, 20));
    batteryparams_trend_t r1 = trendCreate("R1", QColor(30, 110, 200));
    batteryparams_trend_t r2 = trendCreate("R2", QColor(30, 150, 60));

    for(int i = 0; i < cycles.size(); i++)
    {
        if(!cycles[i].socValid) continue;

        if(cycles[i].resistanceValid)
        {
            r0.soc.append(cycles[i].soc);
            r0.value.append(cycles[i].resistance * 1000.0);
        }

        if(!cycles[i].modelValid) continue;

        r1.soc.append(cycles[i].soc);
        r1.value.append(cycles[i].r1 * 1000.0);

        if(cycles[i].model != BATTERYPARAMS_MODEL_SECOND_ORDER) continue;

        r2.soc.append(cycles[i].soc);
        r2.value.append(cycles[i].r2 * 1000.0);
    }

    trends << r0 << r1 << r2;
    trendShow(&resistanceTrendWnd, "Resistance vs SoC", "R [mOhm]", trends);
}

void BatteryParamsWnd::onCapacitanceTrend()
{
    QVector<batteryparams_trend_t> trends;
    batteryparams_trend_t c1 = trendCreate("C1", QColor(30, 110, 200));
    batteryparams_trend_t c2 = trendCreate("C2", QColor(30, 150, 60));

    for(int i = 0; i < cycles.size(); i++)
    {
        if(!cycles[i].socValid) continue;
        if(!cycles[i].modelValid) continue;

        c1.soc.append(cycles[i].soc);
        c1.value.append(cycles[i].c1);

        if(cycles[i].model != BATTERYPARAMS_MODEL_SECOND_ORDER) continue;

        c2.soc.append(cycles[i].soc);
        c2.value.append(cycles[i].c2);
    }

    trends << c1 << c2;
    trendShow(&capacitanceTrendWnd, "Capacitance vs SoC", "C [F]", trends);
}

void BatteryParamsWnd::onOcvTrend()
{
    QVector<batteryparams_trend_t> trends;
    batteryparams_trend_t ocv = trendCreate("OCV", QColor(120, 60, 180));

    for(int i = 0; i < cycles.size(); i++)
    {
        if(!cycles[i].socValid) continue;
        if(!cycles[i].relaxationReached) continue;

        ocv.soc.append(cycles[i].soc);
        ocv.value.append(cycles[i].ocv);
    }

    trends << ocv;
    trendShow(&ocvTrendWnd, "OCV vs SoC", "OCV [V]", trends);
}

void BatteryParamsWnd::onOcvAnalysis()
{
    QVector<double> ocv;
    QVector<double> soc;

    for(int i = 0; i < cycles.size(); i++)
    {
        if(!cycles[i].socValid) continue;
        if(!cycles[i].relaxationReached) continue;

        ocv.append(cycles[i].ocv);
        soc.append(cycles[i].soc);
    }

    if(ocv.size() < 3)
    {
        QMessageBox::warning(this, "OCV analysis",
                             "At least three relaxed cycles with a known state of charge are needed.\n"
                             "Enter the battery capacity in Settings and wait for the cycles where the battery relaxes.");
        return;
    }

    if(ocvAnalysisWnd == NULL)
    {
        ocvAnalysisWnd = new BatteryOcvAnalysisWnd();
        ocvAnalysisWnd->setAttribute(Qt::WA_QuitOnClose, false);
    }

    ocvAnalysisWnd->setPlotSettings(plotSettings);
    ocvAnalysisWnd->setData(soc, ocv);
    ocvAnalysisWnd->show();
    ocvAnalysisWnd->raise();
    ocvAnalysisWnd->activateWindow();
}

void BatteryParamsWnd::onFitQuality()
{
    QVector<batteryparams_cycle_t> firstOrder;
    QVector<batteryparams_cycle_t> secondOrder;
    QVector<batteryfitquality_cycle_t> quality;
    batteryparams_settings_t firstSettings = settings;
    batteryparams_settings_t secondSettings = settings;
    bool parallel;

    for(int i = 0; i < cycles.size(); i++)
    {
        if(!cycles[i].relaxationReached) continue;
        firstOrder.append(cycles[i]);
        secondOrder.append(cycles[i]);
    }

    if(firstOrder.isEmpty())
    {
        QMessageBox::warning(this, "Estimation quality",
                             "No cycle where the battery relaxed, so no model could be fitted.");
        return;
    }

    firstSettings.model = BATTERYPARAMS_MODEL_FIRST_ORDER;
    secondSettings.model = BATTERYPARAMS_MODEL_SECOND_ORDER;

    parallel = parallelGet(firstOrder.size() * 2);

    QProgressDialog progressDialog(QString("%1 cycles detected\nFitting the first order model...").arg(firstOrder.size()),
                                   QString(), 0, firstOrder.size(), this);

    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(0);
    progressDialog.setAutoClose(false);
    progressDialog.setAutoReset(false);
    progressDialog.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progressDialog.setWindowTitle("Estimation quality");
    progressDialog.setMinimumWidth(320);
    progressDialog.setValue(0);
    QApplication::processEvents();

    /*Both models are fitted over the same cycles, so their errors can be put
      side by side*/
    analyzeCycles(&firstOrder, &progressDialog, parallel, firstSettings);

    progressDialog.setLabelText(QString("%1 cycles detected\nFitting the second order model...").arg(secondOrder.size()));
    progressDialog.setValue(0);
    QApplication::processEvents();

    analyzeCycles(&secondOrder, &progressDialog, parallel, secondSettings);

    progressDialog.close();

    for(int i = 0; i < firstOrder.size(); i++)
    {
        batteryfitquality_cycle_t entry;

        entry.index = firstOrder[i].index;
        entry.firstOrderValid = firstOrder[i].modelValid;
        entry.firstOrderRmse = firstOrder[i].fitRmse * 1000.0;
        entry.firstOrderMaxError = firstOrder[i].fitMaxError * 1000.0;
        entry.secondOrderValid = secondOrder[i].modelValid && (secondOrder[i].model == BATTERYPARAMS_MODEL_SECOND_ORDER);
        entry.secondOrderRmse = secondOrder[i].fitRmse * 1000.0;
        entry.secondOrderMaxError = secondOrder[i].fitMaxError * 1000.0;

        quality.append(entry);
    }

    if(fitQualityWnd == NULL)
    {
        fitQualityWnd = new BatteryFitQualityWnd();
        fitQualityWnd->setAttribute(Qt::WA_QuitOnClose, false);
    }

    fitQualityWnd->setPlotSettings(plotSettings);
    fitQualityWnd->setData(quality);
    fitQualityWnd->show();
    fitQualityWnd->raise();
    fitQualityWnd->activateWindow();
}

void BatteryParamsWnd::onRelaxationTime()
{
    QVector<batteryparams_bar_t> bars;

    for(int i = 0; i < cycles.size(); i++)
    {
        batteryparams_bar_t bar;

        if(!cycles[i].relaxationReached) continue;

        bar.index = cycles[i].index;
        bar.value = (cycles[i].relaxationKey - cycles[i].pauseStartKey) / 1000.0;

        bars.append(bar);
    }

    if(bars.isEmpty())
    {
        QMessageBox::warning(this, "Relaxation time", "No cycle where the battery relaxed.");
        return;
    }

    if(relaxationTimeWnd == NULL)
    {
        relaxationTimeWnd = new BatteryParamsBarWnd();
        relaxationTimeWnd->setAttribute(Qt::WA_QuitOnClose, false);
    }

    relaxationTimeWnd->setPlotSettings(plotSettings);
    relaxationTimeWnd->setData("Relaxation time", "Time to relaxation [s]", QColor(30, 150, 60), bars);
    relaxationTimeWnd->show();
    relaxationTimeWnd->raise();
    relaxationTimeWnd->activateWindow();
}

void BatteryParamsWnd::onFitInterval()
{
    QVector<batteryparams_cycle_t> relaxationFit;
    QVector<batteryparams_cycle_t> pauseFit;
    QVector<batteryfitinterval_cycle_t> comparison;
    batteryparams_settings_t relaxationSettings = settings;
    batteryparams_settings_t pauseSettings = settings;
    bool parallel;

    for(int i = 0; i < cycles.size(); i++)
    {
        if(!cycles[i].relaxationReached) continue;
        relaxationFit.append(cycles[i]);
        pauseFit.append(cycles[i]);
    }

    if(relaxationFit.isEmpty())
    {
        QMessageBox::warning(this, "Fit interval", "No cycle where the battery relaxed, so no model could be fitted.");
        return;
    }

    relaxationSettings.fitEnd = BATTERYPARAMS_FIT_END_RELAXATION;
    pauseSettings.fitEnd = BATTERYPARAMS_FIT_END_PAUSE;

    parallel = parallelGet(relaxationFit.size() * 2);

    QProgressDialog progressDialog(QString("%1 cycles detected\nFitting up to the relaxation point...").arg(relaxationFit.size()),
                                   QString(), 0, relaxationFit.size(), this);

    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(0);
    progressDialog.setAutoClose(false);
    progressDialog.setAutoReset(false);
    progressDialog.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progressDialog.setWindowTitle("Fit interval");
    progressDialog.setMinimumWidth(320);
    progressDialog.setValue(0);
    QApplication::processEvents();

    analyzeCycles(&relaxationFit, &progressDialog, parallel, relaxationSettings);

    progressDialog.setLabelText(QString("%1 cycles detected\nFitting up to the end of the pause...").arg(pauseFit.size()));
    progressDialog.setValue(0);
    QApplication::processEvents();

    analyzeCycles(&pauseFit, &progressDialog, parallel, pauseSettings);

    progressDialog.close();

    for(int i = 0; i < relaxationFit.size(); i++)
    {
        batteryfitinterval_cycle_t entry;

        entry.index = relaxationFit[i].index;

        entry.relaxationValid = relaxationFit[i].modelValid;
        entry.relaxationR1 = relaxationFit[i].r1;
        entry.relaxationC1 = relaxationFit[i].c1;
        entry.relaxationR2 = relaxationFit[i].r2;
        entry.relaxationC2 = relaxationFit[i].c2;
        entry.relaxationRmse = relaxationFit[i].fitRmse * 1000.0;
        entry.relaxationMaxError = relaxationFit[i].fitMaxError * 1000.0;

        entry.pauseValid = pauseFit[i].modelValid;
        entry.pauseR1 = pauseFit[i].r1;
        entry.pauseC1 = pauseFit[i].c1;
        entry.pauseR2 = pauseFit[i].r2;
        entry.pauseC2 = pauseFit[i].c2;
        entry.pauseRmse = pauseFit[i].fitRmse * 1000.0;
        entry.pauseMaxError = pauseFit[i].fitMaxError * 1000.0;

        comparison.append(entry);
    }

    if(fitIntervalWnd == NULL)
    {
        fitIntervalWnd = new BatteryFitIntervalWnd();
        fitIntervalWnd->setAttribute(Qt::WA_QuitOnClose, false);
    }

    if(!connectedToFitIntervalWnd)
    {
        connect(fitIntervalWnd, &BatteryFitIntervalWnd::sigSweepRequested, this, &BatteryParamsWnd::onSweepRequested);
        connectedToFitIntervalWnd = true;
    }

    fitIntervalWnd->setPlotSettings(plotSettings);
    fitIntervalWnd->setData(comparison);
    fitIntervalWnd->show();
    fitIntervalWnd->raise();
    fitIntervalWnd->activateWindow();
}

/*Every sweep point is a complete analysis of all cycles with a different
  relaxation criterion, so the work goes through the same parallel path as the
  other analyses*/
void BatteryParamsWnd::onSweepRequested(batteryfitinterval_sweep_t mode, double fixedValue,
                                        double from, double to, int steps, bool compare)
{
    QVector<batteryparams_cycle_t> source;
    QVector<double> sweepValues;
    QVector<double> rmseValues;
    QVector<double> maxErrorValues;
    QVector<double> relaxedValues;
    QVector<unsigned int> cycleIndexes;
    QVector<QVector<double>> cycleRmse;
    QVector<QVector<double>> cycleMaxError;
    QVector<double> relaxationTimeValues;
    QVector<QVector<double>> cycleRelaxationTime;
    double referenceRmse = 0;
    double referenceMaxError = 0;
    bool parallel;

    for(int i = 0; i < cycles.size(); i++)
    {
        source.append(cycles[i]);
    }

    if(source.isEmpty()) return;

    for(int i = 0; i < source.size(); i++)
    {
        cycleIndexes.append(source[i].index);
        cycleRmse.append(QVector<double>());
        cycleMaxError.append(QVector<double>());
        cycleRelaxationTime.append(QVector<double>());
    }
    if(steps < 2) steps = 2;
    if(to <= from) to = from + 1;

    parallel = parallelGet(source.size() * steps);

    QProgressDialog progressDialog(QString("Sweeping the relaxation criterion over %1 points...").arg(steps),
                                   QString(), 0, steps + (compare ? 1 : 0), this);

    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(0);
    progressDialog.setAutoClose(false);
    progressDialog.setAutoReset(false);
    progressDialog.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progressDialog.setWindowTitle("Relaxation sweep");
    progressDialog.setMinimumWidth(340);
    progressDialog.setValue(0);
    QApplication::processEvents();

    for(int step = 0; step < steps; step++)
    {
        QVector<batteryparams_cycle_t> target = source;
        batteryparams_settings_t stepSettings = settings;
        double value = from + (to - from) * (double)step / (steps - 1);
        double rmseSum = 0;
        double maxErrorSum = 0;
        double relaxationTimeSum = 0;
        int validNo = 0;

        stepSettings.fitEnd = BATTERYPARAMS_FIT_END_RELAXATION;

        if(mode == BATTERYFITINTERVAL_SWEEP_THRESHOLD)
        {
            stepSettings.relaxationThreshold = value;
            stepSettings.relaxationWindow = fixedValue;
        }
        else
        {
            stepSettings.relaxationThreshold = fixedValue;
            stepSettings.relaxationWindow = value;
        }

        progressDialog.setLabelText(QString("Sweep point %1 of %2\n%3 %4 with %5 %6 held")
                                    .arg(step + 1).arg(steps)
                                    .arg(value, 0, 'f', 2)
                                    .arg(mode == BATTERYFITINTERVAL_SWEEP_THRESHOLD ? "mV" : "s")
                                    .arg(fixedValue, 0, 'f', 2)
                                    .arg(mode == BATTERYFITINTERVAL_SWEEP_THRESHOLD ? "s" : "mV"));
        QApplication::processEvents();

        analyzeCycles(&target, NULL, parallel, stepSettings);

        for(int i = 0; i < target.size(); i++)
        {
            if(!target[i].modelValid)
            {
                /*Negative marks a cycle that could not be fitted with this criterion*/
                cycleRmse[i].append(-1);
                cycleMaxError[i].append(-1);
                cycleRelaxationTime[i].append(-1);
                continue;
            }

            cycleRmse[i].append(target[i].fitRmse * 1000.0);
            cycleMaxError[i].append(target[i].fitMaxError * 1000.0);
            cycleRelaxationTime[i].append((target[i].relaxationKey - target[i].pauseStartKey) / 1000.0);

            rmseSum += target[i].fitRmse * 1000.0;
            maxErrorSum += target[i].fitMaxError * 1000.0;
            relaxationTimeSum += (target[i].relaxationKey - target[i].pauseStartKey) / 1000.0;
            validNo++;
        }

        /*A criterion so tight that no cycle relaxes has no error to report, so the
          point is left out instead of being drawn as zero*/
        if(validNo > 0)
        {
            sweepValues.append(value);
            rmseValues.append(rmseSum / validNo);
            maxErrorValues.append(maxErrorSum / validNo);
            relaxationTimeValues.append(relaxationTimeSum / validNo);
            relaxedValues.append(validNo);
        }
        else
        {
            /*Point is dropped, so the per cycle rows stay aligned with it*/
            for(int i = 0; i < cycleRmse.size(); i++)
            {
                cycleRmse[i].removeLast();
                cycleMaxError[i].removeLast();
                cycleRelaxationTime[i].removeLast();
            }
        }

        progressDialog.setValue(step + 1);
        QApplication::processEvents();
    }

    if(compare)
    {
        QVector<batteryparams_cycle_t> target = source;
        batteryparams_settings_t referenceSettings = settings;
        double rmseSum = 0;
        double maxErrorSum = 0;
        int validNo = 0;

        referenceSettings.fitEnd = BATTERYPARAMS_FIT_END_PAUSE;

        progressDialog.setLabelText("Fitting the reference over the whole pause...");
        QApplication::processEvents();

        analyzeCycles(&target, NULL, parallel, referenceSettings);

        for(int i = 0; i < target.size(); i++)
        {
            if(!target[i].modelValid) continue;

            rmseSum += target[i].fitRmse * 1000.0;
            maxErrorSum += target[i].fitMaxError * 1000.0;
            validNo++;
        }

        if(validNo > 0)
        {
            referenceRmse = rmseSum / validNo;
            referenceMaxError = maxErrorSum / validNo;
        }

        progressDialog.setValue(steps + 1);
    }

    progressDialog.close();

    fitIntervalWnd->setSweep(sweepValues, rmseValues, maxErrorValues, relaxedValues,
                             referenceRmse, referenceMaxError, cycleIndexes, cycleRmse, cycleMaxError,
                             relaxationTimeValues, cycleRelaxationTime);
}

void BatteryParamsWnd::onExport()
{
    QString suggested = profileName.isEmpty() ? "battery_params.csv" : profileName + "_battery_params.csv";
    QString path;
    QFile file;

    if(cycles.isEmpty())
    {
        QMessageBox::warning(this, "Export", "No finished cycles to export");
        return;
    }

    path = QFileDialog::getSaveFileName(this, "Export battery parameters", suggested, "CSV files (*.csv);;All files (*)");
    if(path.isEmpty()) return;

    file.setFileName(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Export", "Cannot open " + path);
        return;
    }

    QTextStream stream(&file);
    stream << "Cycle,Start [ms],End [ms],Q [mAh],Q total [mAh],SoC used [%],SoC [%],OCV [V],R1 [mOhm],C1 [F],R2 [mOhm],C2 [F],RMSE [mV],Max err [mV],V pulse end [V],V pause start [V],dV [mV],I pulse end [mA],I pause start [mA],dI [mA],R [mOhm]\n";
    for(int i = 0; i < cycles.size(); i++)
    {
        stream << cycles[i].index << ","
               << QString::number(cycles[i].pulseStartKey, 'f', 3) << ","
               << QString::number(cycles[i].pauseEndKey, 'f', 3) << ","
               << QString::number(cycles[i].charge, 'f', 3) << ","
               << QString::number(cycles[i].chargeTotal, 'f', 3) << ","
               << (cycles[i].socValid ? QString::number(cycles[i].socUsed, 'f', 2) : "") << ","
               << (cycles[i].socValid ? QString::number(cycles[i].soc, 'f', 2) : "") << ","
               << (cycles[i].relaxationReached ? QString::number(cycles[i].ocv, 'f', 4) : "") << ","
               << (cycles[i].modelValid ? QString::number(cycles[i].r1 * 1000.0, 'f', 2) : "") << ","
               << (cycles[i].modelValid ? QString::number(cycles[i].c1, 'f', 1) : "") << ","
               << ((cycles[i].modelValid && cycles[i].model == BATTERYPARAMS_MODEL_SECOND_ORDER) ? QString::number(cycles[i].r2 * 1000.0, 'f', 2) : "") << ","
               << ((cycles[i].modelValid && cycles[i].model == BATTERYPARAMS_MODEL_SECOND_ORDER) ? QString::number(cycles[i].c2, 'f', 1) : "") << ","
               << (cycles[i].modelValid ? QString::number(cycles[i].fitRmse * 1000.0, 'f', 3) : "") << ","
               << (cycles[i].modelValid ? QString::number(cycles[i].fitMaxError * 1000.0, 'f', 3) : "") << ","
               << QString::number(cycles[i].pulseEndVoltage, 'f', 4) << ","
               << QString::number(cycles[i].pauseStartVoltage, 'f', 4) << ","
               << QString::number(cycles[i].deltaVoltage * 1000.0, 'f', 2) << ","
               << QString::number(cycles[i].pulseEndCurrent, 'f', 2) << ","
               << QString::number(cycles[i].pauseStartCurrent, 'f', 2) << ","
               << QString::number(cycles[i].deltaCurrent, 'f', 2) << ","
               << (cycles[i].resistanceValid ? QString::number(cycles[i].resistance * 1000.0, 'f', 2) : "") << "\n";
    }
    file.close();
}
