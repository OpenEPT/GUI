#include "dataanalyzerprofile.h"

#include <QVBoxLayout>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QDir>
#include <QProgressDialog>

#define PLOT_MINIMUM_SIZE_HEIGHT 100
#define PLOT_MINIMUM_SIZE_WIDTH 500

DataAnalyzerProfile::DataAnalyzerProfile(QString aWsDirPath, QString aProfileName, QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    setFont(defaultFont);
    mainLayout->setContentsMargins(2, 2, 2, 2);

    wsDirPath = aWsDirPath;
    profileName = aProfileName;
    epEnabledFlag = false;
    graphLoad = false;
    progressDialog = NULL;
    statisticsProgressDialog = NULL;

    plotsToolBar = new QToolBar(this);
    plotsToolBar->setIconSize(QSize(24, 24));
    QAction *saveAllPlotsAction = plotsToolBar->addAction(QIcon(QPixmap(":/images/NewSet/save.png")), "Save all plots");
    saveAllPlotsAction->setToolTip("Save Voltage, Current and Consumption plots (current view) to a folder");
    connect(saveAllPlotsAction, SIGNAL(triggered(bool)), this, SLOT(onSaveAllPlots()));
    QAction *genStatisticsAction = plotsToolBar->addAction(QIcon(QPixmap(":/images/NewSet/analysis.png")), "Gen statistics");
    genStatisticsAction->setToolTip("Generate consumption statistics for segments between \"<name> Start\" and \"<name> Stop\" markers");
    connect(genStatisticsAction, SIGNAL(triggered(bool)), this, SLOT(onGenerateStatistics()));
    QAction *batteryAnalyzerAction = plotsToolBar->addAction(QIcon(QPixmap(":/images/NewSet/loadProfiler.png")), "Battery analyzer");
    batteryAnalyzerAction->setToolTip("Extract battery parameters from \"Pulse Start / Pulse End / Pause Start / Pause End\" marker cycles");
    connect(batteryAnalyzerAction, SIGNAL(triggered(bool)), this, SLOT(onBatteryAnalyzer()));
    detachAction = plotsToolBar->addAction(QIcon(QPixmap(":/images/NewSet/expand.png")), "Attach");
    detachAction->setToolTip("Show this profile back as a tab of the Data Analyzer");
    detachAction->setVisible(false);
    connect(detachAction, SIGNAL(triggered(bool)), this, SLOT(onDetachClicked()));
    mainLayout->addWidget(plotsToolBar);

    statisticsWnd = new DataAnalyzerStatisticsWnd();
    batteryParamsWnd = NULL;
    statisticsThread = new QThread(this);
    statisticsWorker = new DataAnalyzerStatisticsWorker();
    statisticsWorker->moveToThread(statisticsThread);
    statisticsThread->setObjectName("OpenEPT - Data Analyzer statistics");
    connect(this, SIGNAL(sigComputeStatistics(QVector<double>,QVector<double>,QVector<double>,QVector<double>,QVector<QPair<QString, int>>)),
            statisticsWorker, SLOT(onComputeStatistics(QVector<double>,QVector<double>,QVector<double>,QVector<double>,QVector<QPair<QString, int>>)), Qt::QueuedConnection);
    connect(statisticsWorker, SIGNAL(sigStatisticsFinished(QVector<dataanalyzer_segment_stat_t>,QVector<dataanalyzer_point_marker_t>,dataanalyzer_segment_stat_t,QStringList)),
            this, SLOT(onStatisticsFinished(QVector<dataanalyzer_segment_stat_t>,QVector<dataanalyzer_point_marker_t>,dataanalyzer_segment_stat_t,QStringList)), Qt::QueuedConnection);
    connect(statisticsWorker, SIGNAL(sigStatisticsProgress(int,QString)), this, SLOT(onStatisticsProgress(int,QString)), Qt::QueuedConnection);
    connect(statisticsWnd, SIGNAL(sigSegmentSelected(QString,int,int)), this, SLOT(onStatisticsSegmentSelected(QString,int,int)));
    connect(statisticsWnd, SIGNAL(sigMarkerVisibilityChanged(QVector<dataanalyzer_segment_stat_t>,QVector<bool>)), this, SLOT(onStatisticsMarkerVisibilityChanged(QVector<dataanalyzer_segment_stat_t>,QVector<bool>)));
    statisticsThread->start();

    mainWindow = new QMainWindow(this);
    mainWindow->setWindowFlags(Qt::Widget);
    mainWindow->setDockNestingEnabled(true);
    mainLayout->addWidget(mainWindow);

    createVoltageSubWin();
    createCurrentSubWin();
    createConsumptionSubWin();

    thread = new QThread(this);
    dataProcesingClass = new DataAnalyzerWorker();
    dataProcesingClass->moveToThread(thread);
    thread->setObjectName("OpenEPT - Data Analyzer");

    connect(this, SIGNAL(processVolCurConRequest(QString,QString)),
            dataProcesingClass, SLOT(processVoltCurConData(QString,QString)), Qt::QueuedConnection);
    connect(dataProcesingClass, SIGNAL(processingVolCurConFinished(QVector<QVector<double>>, QVector<QVector<double>>)),
            this, SLOT(processingVolCurConDone(QVector<QVector<double>>, QVector<QVector<double>>)), Qt::QueuedConnection);
    connect(this, SIGNAL(processEPRequest(QString,QString)),
            dataProcesingClass, SLOT(processEPData(QString,QString)), Qt::QueuedConnection);
    connect(dataProcesingClass, SIGNAL(processingEPFinished(QVector<QPair<QString, int>>)),
            this, SLOT(processingEPDone(QVector<QPair<QString, int>>)), Qt::QueuedConnection);
    connect(dataProcesingClass, SIGNAL(progressUpdated(int)),
            this, SLOT(updateProgress(int)), Qt::QueuedConnection);
    connect(dataProcesingClass, SIGNAL(updateProgressText(QString)),
            this, SLOT(updateProgressText(QString)), Qt::QueuedConnection);

    thread->start();
}

DataAnalyzerProfile::~DataAnalyzerProfile()
{
    statisticsThread->quit();
    statisticsThread->wait();
    thread->quit();
    thread->wait();
    delete statisticsWorker;
    delete dataProcesingClass;
    delete statisticsWnd;
}

QString DataAnalyzerProfile::getProfileName()
{
    return profileName;
}

void DataAnalyzerProfile::setDetached(bool detached)
{
    detachAction->setVisible(detached);
}

void DataAnalyzerProfile::onDetachClicked()
{
    emit sigDockStateToggleRequested();
}

void DataAnalyzerProfile::createVoltageSubWin()
{
    QDockWidget *dockWidget = new QDockWidget("Voltage", this);
    QWidget *contentWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);

    dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
    layout->setContentsMargins(2, 2, 2, 2);

    voltageChart             = new Plot(PLOT_MINIMUM_SIZE_WIDTH/2, PLOT_MINIMUM_SIZE_HEIGHT, false);
    voltageChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    voltageChart->setTitle("Voltage");
    voltageChart->setYLabel("[V]");
    voltageChart->setXLabel("[ms]");

    layout->addWidget(voltageChart);
    contentWidget->setLayout(layout);
    dockWidget->setWidget(contentWidget);
    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
}

void DataAnalyzerProfile::createCurrentSubWin()
{
    QDockWidget *dockWidget = new QDockWidget("Current", this);
    QWidget *contentWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);

    dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
    layout->setContentsMargins(2, 2, 2, 2);

    currentChart             = new Plot(PLOT_MINIMUM_SIZE_WIDTH/2, PLOT_MINIMUM_SIZE_HEIGHT, false);
    currentChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    currentChart->setTitle("Current");
    currentChart->setYLabel("[mA]");
    currentChart->setXLabel("[ms]");

    layout->addWidget(currentChart);
    contentWidget->setLayout(layout);
    dockWidget->setWidget(contentWidget);
    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
}

void DataAnalyzerProfile::createConsumptionSubWin()
{
    QDockWidget *dockWidget = new QDockWidget("Consumption", this);
    QWidget *contentWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);

    dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
    layout->setContentsMargins(2, 2, 2, 2);

    consumptionChart             = new Plot(PLOT_MINIMUM_SIZE_WIDTH/2, PLOT_MINIMUM_SIZE_HEIGHT, false);
    consumptionChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    consumptionChart->setTitle("Consumption");
    consumptionChart->setYLabel("[mA]");
    consumptionChart->setXLabel("[ms]");

    connect(voltageChart, SIGNAL(sigXRangeChanged(QCPRange)), this, SLOT(onPlotXRangeChanged(QCPRange)));
    connect(currentChart, SIGNAL(sigXRangeChanged(QCPRange)), this, SLOT(onPlotXRangeChanged(QCPRange)));
    connect(consumptionChart, SIGNAL(sigXRangeChanged(QCPRange)), this, SLOT(onPlotXRangeChanged(QCPRange)));

    layout->addWidget(consumptionChart);
    contentWidget->setLayout(layout);
    dockWidget->setWidget(contentWidget);
    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
}

void DataAnalyzerProfile::onPlotXRangeChanged(QCPRange range)
{
    Plot *source = qobject_cast<Plot*>(sender());
    Plot *plots[3] = {voltageChart, currentChart, consumptionChart};

    for(int i = 0; i < 3; i++)
    {
        if(plots[i] == source) continue;
        plots[i]->setXRangeSynced(range);
    }
}

void DataAnalyzerProfile::startLoading()
{
    QString summaryFilePath = wsDirPath + "/" + profileName + "/OpenEPT.txt";
    QVector<QPair<QString, QString>>    summaryInfo = parseSummaryFile(summaryFilePath);
    QString epEnabled   = getValueForKey(summaryInfo, "EP Enabled");

    epEnabledFlag = (epEnabled == "1");

    if(epEnabledFlag)
    {
        dataProcesingClass->setLimits(3);
    }
    else
    {
        dataProcesingClass->setLimits(2);
    }

    progressDialog = new QProgressDialog("Loading Data... ", "Cancel", 0, 100, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0);
    progressDialog->setAutoClose(true);
    progressDialog->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progressDialog->setWindowTitle("Load OpenEPT Data");
    progressDialog->show();

    emit processVolCurConRequest(wsDirPath, profileName);
}

void DataAnalyzerProfile::processingVolCurConDone(QVector<QVector<double> > vc, QVector<QVector<double> > cons)
{
    if(graphLoad)
    {
        voltageChart->clear();
        currentChart->clear();
        consumptionChart->clear();
        graphLoad = false;
    }

    voltageChart->setData(vc[0],vc[1]);
    currentChart->setData(vc[2],vc[3]);
    consumptionChart->setData(cons[0], cons[1]);

    loadedVoltage = vc[0];
    loadedVoltageKeys = vc[1];
    loadedCurrent = vc[2];
    loadedCurrentKeys = vc[3];
    loadedMarkers.clear();

    graphLoad = true;

    if(epEnabledFlag)
    {
        emit processEPRequest(wsDirPath, profileName);
    }
    else
    {
        if(progressDialog) progressDialog->close();
    }
}

void DataAnalyzerProfile::processingEPDone(QVector<QPair<QString, int> > epData)
{
    loadedMarkers = epData;
    consumptionChart->scatterAddGraph();
    voltageChart->scatterAddGraph();
    currentChart->scatterAddGraph();
    applyMarkersToPlots(epData);
    if(progressDialog) progressDialog->close();
}

void DataAnalyzerProfile::updateProgress(int percentage)
{
    if(progressDialog == NULL) return;
    progressDialog->setValue(percentage);
    if(percentage >= 100) progressDialog->close();
}

void DataAnalyzerProfile::updateProgressText(QString text)
{
    if(progressDialog == NULL) return;
    progressDialog->setLabelText(text);
}

void DataAnalyzerProfile::onGenerateStatistics()
{
    if(!graphLoad)
    {
        QMessageBox::warning(this, "Statistics", "No consumption profile loaded");
        return;
    }
    if(loadedMarkers.isEmpty())
    {
        QMessageBox::warning(this, "Statistics", "Loaded profile has no energy point markers (\"<name> Start\" / \"<name> Stop\" pairs are required)");
        return;
    }

    if(statisticsProgressDialog == NULL)
    {
        statisticsProgressDialog = new QProgressDialog("Generating statistics... ", QString(), 0, 100, this);
        statisticsProgressDialog->setWindowModality(Qt::WindowModal);
        statisticsProgressDialog->setMinimumDuration(0);
        statisticsProgressDialog->setAutoClose(false);
        statisticsProgressDialog->setAutoReset(false);
        statisticsProgressDialog->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        statisticsProgressDialog->setWindowTitle("Consumption statistics");
    }
    statisticsProgressDialog->setLabelText("Generating statistics... ");
    statisticsProgressDialog->setValue(0);
    statisticsProgressDialog->show();

    emit sigComputeStatistics(loadedVoltage, loadedVoltageKeys, loadedCurrent, loadedCurrentKeys, loadedMarkers);
}

void DataAnalyzerProfile::onBatteryAnalyzer()
{
    BatteryParamsExtraction extraction;
    QStringList requiredMarkers;
    int cycleNo = 0;

    if(!graphLoad)
    {
        QMessageBox::warning(this, "Battery analyzer", "No consumption profile loaded");
        return;
    }

    requiredMarkers << BATTERYPARAMS_DEFAULT_PULSE_START_MARKER
                    << BATTERYPARAMS_DEFAULT_PULSE_END_MARKER
                    << BATTERYPARAMS_DEFAULT_PAUSE_START_MARKER
                    << BATTERYPARAMS_DEFAULT_PAUSE_END_MARKER;

    for(int i = 0; i < requiredMarkers.size(); i++)
    {
        bool found = false;

        for(int j = 0; j < loadedMarkers.size(); j++)
        {
            if(loadedMarkers[j].first.trimmed() != requiredMarkers[i]) continue;
            found = true;
            break;
        }

        if(found) continue;

        QMessageBox::warning(this, "Battery analyzer",
                             "Loaded profile has no \"" + requiredMarkers[i] + "\" marker.\n"
                             "Battery parameters can be extracted only from a profile recorded with "
                             "the \"Bat Param Extraction\" load mode.");
        return;
    }

    /*Capacity is stored with the profile, so the offline analysis can show the
      used state of charge the same way the live one does*/
    {
        QVector<QPair<QString, QString>> summaryInfo = parseSummaryFile(wsDirPath + "/" + profileName + "/OpenEPT.txt");
        double capacity = getValueForKey(summaryInfo, "Battery capacity [mAh]").toDouble();
        batteryparams_settings_t settings = extraction.getSettings();

        if(capacity > 0)
        {
            settings.capacity = capacity;
            extraction.setSettings(settings);
        }
    }

    if(batteryParamsWnd == NULL)
    {
        batteryParamsWnd = new BatteryParamsWnd();
        batteryParamsWnd->setAttribute(Qt::WA_QuitOnClose, false);
    }

    batteryParamsWnd->setProfileName(profileName);
    batteryParamsWnd->setSettings(extraction.getSettings());
    batteryParamsWnd->onClear();

    /*Cycles are detected first and fitted afterwards, so the fitting can be
      spread over all cores. The dialog is stepped manually because everything
      runs from this thread*/
    extraction.setAnalysisDeferred(true);

    int detectedCycleNo = 0;

    for(int i = 0; i < loadedMarkers.size(); i++)
    {
        if(loadedMarkers[i].first.trimmed() != BATTERYPARAMS_DEFAULT_PULSE_START_MARKER) continue;
        detectedCycleNo++;
    }

    QProgressDialog progressDialog(QString("%1 cycles detected\nReading samples...").arg(detectedCycleNo),
                                   QString(), 0, loadedMarkers.size() + 1, this);

    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(0);
    progressDialog.setAutoClose(false);
    progressDialog.setAutoReset(false);
    progressDialog.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progressDialog.setWindowTitle("Battery analyzer");
    progressDialog.setMinimumWidth(320);
    progressDialog.setValue(0);
    QApplication::processEvents();

    extraction.onNewSamplesReceived(loadedVoltage, loadedCurrent, loadedVoltageKeys, loadedCurrentKeys);

    progressDialog.setValue(1);
    QApplication::processEvents();

    for(int i = 0; i < loadedMarkers.size(); i++)
    {
        QString marker = loadedMarkers[i].first.trimmed();

        if(marker == BATTERYPARAMS_DEFAULT_PAUSE_END_MARKER)
        {
            progressDialog.setLabelText(QString("%1 cycles detected\nSplitting cycle %2 of %1...")
                                        .arg(detectedCycleNo)
                                        .arg(extraction.getCycles().size() + 1));
            QApplication::processEvents();
        }

        extraction.onNewMarkerReceived(0, loadedMarkers[i].second, marker);

        progressDialog.setValue(i + 2);
        QApplication::processEvents();
    }

    cycleNo = extraction.getCycles().size();

    if(cycleNo == 0)
    {
        progressDialog.close();
        QMessageBox::warning(this, "Battery analyzer", "No complete pulse/pause cycle found in the loaded profile");
        return;
    }

    progressDialog.setRange(0, cycleNo);
    progressDialog.setValue(0);

    batteryParamsWnd->addCycles(extraction.getCycles(), &progressDialog);

    progressDialog.close();

    batteryParamsWnd->show();
    batteryParamsWnd->raise();
    batteryParamsWnd->activateWindow();
}

void DataAnalyzerProfile::onStatisticsProgress(int percentage, QString text)
{
    if(statisticsProgressDialog == NULL) return;
    statisticsProgressDialog->setLabelText(text);
    statisticsProgressDialog->setValue(percentage);
}

void DataAnalyzerProfile::onStatisticsFinished(QVector<dataanalyzer_segment_stat_t> stats, QVector<dataanalyzer_point_marker_t> points, dataanalyzer_segment_stat_t total, QStringList warnings)
{
    if(statisticsProgressDialog != NULL) statisticsProgressDialog->hide();

    if(stats.isEmpty() && points.isEmpty() && warnings.isEmpty())
    {
        QMessageBox::information(this, "Statistics", "No \"<name> Start\" / \"<name> Stop\" marker pairs found");
        return;
    }
    statisticsWnd->setStatistics(profileName, stats, points, total, warnings);
    statisticsWnd->show();
    statisticsWnd->raise();
    statisticsWnd->activateWindow();
}

void DataAnalyzerProfile::applyMarkersToPlots(QVector<QPair<QString, int>> markers)
{
    consumptionChart->scatterClearMarkers();
    voltageChart->scatterClearMarkers();
    currentChart->scatterClearMarkers();

    consumptionChart->scatterAddAllDataWithName(markers);
    voltageChart->scatterAddAllDataWithName(markers);
    currentChart->scatterAddAllDataWithName(markers);
}

void DataAnalyzerProfile::onStatisticsMarkerVisibilityChanged(QVector<dataanalyzer_segment_stat_t> segments, QVector<bool> expanded)
{
    QVector<QPair<QString, int>> visibleMarkers;

    if(!graphLoad) return;

    for(int i = 0; i < loadedMarkers.size(); i++)
    {
        QString name = loadedMarkers[i].first.trimmed();
        int index = loadedMarkers[i].second;
        int owner = -1;

        for(int k = 0; k < segments.size(); k++)
        {
            if(index < segments[k].startIndex || index > segments[k].endIndex) continue;
            if(name.compare(segments[k].name + " Start", Qt::CaseInsensitive) != 0 &&
               name.compare(segments[k].name + " Stop", Qt::CaseInsensitive) != 0) continue;
            owner = k;
            break;
        }

        if(owner < 0)
        {
            for(int k = 0; k < segments.size(); k++)
            {
                if(index < segments[k].startIndex || index > segments[k].endIndex) continue;
                if(owner < 0 || (segments[k].endIndex - segments[k].startIndex) < (segments[owner].endIndex - segments[owner].startIndex))
                {
                    owner = k;
                }
            }
        }

        if(owner >= 0 && expanded.value(owner, true) == false) continue;

        visibleMarkers.append(loadedMarkers[i]);
    }

    applyMarkersToPlots(visibleMarkers);
}

void DataAnalyzerProfile::onStatisticsSegmentSelected(QString name, int startIndex, int endIndex)
{
    double min;
    double max;

    Q_UNUSED(name);

    if(!graphLoad) return;
    if(startIndex < 0 || endIndex >= loadedVoltageKeys.size() || endIndex <= startIndex) return;

    min = loadedVoltageKeys[startIndex];
    max = loadedVoltageKeys[endIndex];
    voltageChart->zoomToKeyRange(min, max);
    currentChart->zoomToKeyRange(min, max);
    consumptionChart->zoomToKeyRange(min, max);

    window()->raise();
    window()->activateWindow();
}

void DataAnalyzerProfile::onSaveAllPlots()
{
    Plot *plots[3] = {voltageChart, currentChart, consumptionChart};
    QString defaultDir = wsDirPath;
    QString dir;
    QString prefix;
    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QStringList failed;

    if(!graphLoad)
    {
        QMessageBox::warning(this, "Save plots", "No consumption profile loaded");
        return;
    }
    if(!profileName.isEmpty())
    {
        defaultDir = wsDirPath + "/" + profileName;
    }

    dir = QFileDialog::getExistingDirectory(this, "Select folder for plot images", defaultDir);
    if(dir.isEmpty()) return;

    prefix = profileName.isEmpty() ? "profile" : profileName.section('/', -1).toLower().replace(' ', '_');

    for(int i = 0; i < 3; i++)
    {
        QString base = dir + "/oept_" + prefix + "_" + plots[i]->getTitle().toLower().replace(' ', '_') + "_" + timeStamp;
        if(!plots[i]->saveImageToFile(base + ".png")) failed << base + ".png";
        if(!plots[i]->saveImageToFile(base + ".svg")) failed << base + ".svg";
    }

    if(!failed.isEmpty())
    {
        QMessageBox::warning(this, "Save plots", "Unable to save:\n" + failed.join("\n"));
    }
}

QString DataAnalyzerProfile::getValueForKey(const QVector<QPair<QString, QString>> &parsedData, const QString &key)
{
    for(const auto &pair : parsedData)
    {
        if(pair.first == key) return pair.second;
    }
    return "";
}

QVector<QPair<QString, QString>> DataAnalyzerProfile::parseSummaryFile(const QString &filePath)
{
    QVector<QPair<QString, QString>> data;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open file:" << filePath;
        return data;
    }

    QTextStream in(&file);

    in.readLine();
    in.readLine();

    while(!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if(line.isEmpty()) continue;

        QStringList parts = line.split(":", Qt::SkipEmptyParts);
        if(parts.size() == 2)
        {
            data.append(qMakePair(parts[0].trimmed(), parts[1].trimmed()));
        }
    }

    file.close();
    return data;
}
