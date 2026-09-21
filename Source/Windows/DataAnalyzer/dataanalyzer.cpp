#include "dataanalyzer.h"
#include "ui_dataanalyzer.h"
#include <QFileDialog>
#include <math.h>
#include <QMessageBox>
#include <QDateTime>


#define PLOT_MINIMUM_SIZE_HEIGHT 100
#define PLOT_MINIMUM_SIZE_WIDTH 500

DataAnalyzer::DataAnalyzer(QWidget *parent, QString aWsDirPath) :
    QWidget(parent),
    ui(new Ui::DataAnalyzer)
{
    ui->setupUi(this);    

    wsDirPath = aWsDirPath;

    QFont defaultFont("Arial", 10); // Set desired font and size
    setFont(defaultFont);

    // Set up the main layout for DataAnalyzer
    QVBoxLayout *mainLayout = new QVBoxLayout(this);


    // Create a horizontal layout for the button and line edit
    QHBoxLayout *topLayout = new QHBoxLayout;
    QPushButton *reloadProfileNamesPushb = new QPushButton();
    QPushButton *processFilePushb = new QPushButton();
    QPushButton *deleteProfilePushb = new QPushButton();


    detectedProfilesLabe    = new QLabel("Detect consumption profiles", this);
    detectedProfilesLabe->setFixedSize(260, 30);
    detectedProfilesLabe->setFont(defaultFont);

    consumptionProfilesCB = new QComboBox();
    consumptionProfilesCB->setFont(defaultFont);
    consumptionProfilesCB->setFixedSize(150, 30);

    connect(consumptionProfilesCB, SIGNAL(currentIndexChanged(int)), this, SLOT(onConsumptionProfileChanged(int)));
    realoadConsumptionProfiles();

    QPixmap buttonIconPng(":/images/NewSet/reload.png");
    QIcon buttonIcon(buttonIconPng);
    reloadProfileNamesPushb->setIcon(buttonIcon);
    reloadProfileNamesPushb->setIconSize(QSize(30,30));
    reloadProfileNamesPushb->setToolTip("Reload consumption profiles");
    reloadProfileNamesPushb->setFixedSize(30, 30);

    connect(reloadProfileNamesPushb, SIGNAL(clicked(bool)), this, SLOT(onRealoadConsumptionProfiles()));

    QPixmap processIconPng(":/images/NewSet/load.png");
    QIcon processIcon(processIconPng);
    processFilePushb->setIcon(processIcon);
    processFilePushb->setIconSize(QSize(30,30));
    processFilePushb->setToolTip("Process consumption profile data");
    processFilePushb->setFixedSize(30, 30);

    connect(processFilePushb, SIGNAL(clicked(bool)), this, SLOT(onLoadConsumptionProfileData()));

    deleteProfilePushb->setIcon(QIcon(QPixmap(":/images/NewSet/stopHand.png")));
    deleteProfilePushb->setIconSize(QSize(24,24));
    deleteProfilePushb->setToolTip("Delete selected consumption profile from disk");
    deleteProfilePushb->setFixedSize(30, 30);

    connect(deleteProfilePushb, SIGNAL(clicked(bool)), this, SLOT(onDeleteConsumptionProfile()));

    // Add the button and line edit to the horizontal layout
    topLayout->addWidget(detectedProfilesLabe);
    topLayout->addWidget(consumptionProfilesCB);
    topLayout->addWidget(reloadProfileNamesPushb);
    topLayout->addWidget(processFilePushb);
    topLayout->addSpacing(20);
    topLayout->addWidget(deleteProfilePushb);
    topLayout->addStretch();

    mainLayout->addLayout(topLayout);

    plotsToolBar = new QToolBar(this);
    plotsToolBar->setIconSize(QSize(24, 24));
    QAction *saveAllPlotsAction = plotsToolBar->addAction(QIcon(QPixmap(":/images/NewSet/save.png")), "Save all plots");
    saveAllPlotsAction->setToolTip("Save Voltage, Current and Consumption plots (current view) to a folder");
    connect(saveAllPlotsAction, SIGNAL(triggered(bool)), this, SLOT(onSaveAllPlots()));
    QAction *genStatisticsAction = plotsToolBar->addAction("Gen statistics");
    genStatisticsAction->setToolTip("Generate consumption statistics for segments between \"<name> Start\" and \"<name> Stop\" markers");
    connect(genStatisticsAction, SIGNAL(triggered(bool)), this, SLOT(onGenerateStatistics()));
    mainLayout->addWidget(plotsToolBar);

    statisticsWnd = new DataAnalyzerStatisticsWnd();
    qRegisterMetaType<QVector<dataanalyzer_segment_stat_t>>("QVector<dataanalyzer_segment_stat_t>");
    qRegisterMetaType<QVector<QPair<QString, int>>>("QVector<QPair<QString, int>>");
    statisticsThread = new QThread(this);
    statisticsWorker = new DataAnalyzerStatisticsWorker();
    statisticsWorker->moveToThread(statisticsThread);
    statisticsThread->setObjectName("OpenEPT - Data Analyzer statistics");
    connect(this, SIGNAL(sigComputeStatistics(QVector<double>,QVector<double>,QVector<double>,QVector<double>,QVector<QPair<QString, int>>)),
            statisticsWorker, SLOT(onComputeStatistics(QVector<double>,QVector<double>,QVector<double>,QVector<double>,QVector<QPair<QString, int>>)), Qt::QueuedConnection);
    qRegisterMetaType<dataanalyzer_segment_stat_t>("dataanalyzer_segment_stat_t");
    qRegisterMetaType<QVector<dataanalyzer_point_marker_t>>("QVector<dataanalyzer_point_marker_t>");
    connect(statisticsWorker, SIGNAL(sigStatisticsFinished(QVector<dataanalyzer_segment_stat_t>,QVector<dataanalyzer_point_marker_t>,dataanalyzer_segment_stat_t,QStringList)),
            this, SLOT(onStatisticsFinished(QVector<dataanalyzer_segment_stat_t>,QVector<dataanalyzer_point_marker_t>,dataanalyzer_segment_stat_t,QStringList)), Qt::QueuedConnection);
    statisticsThread->start();
    connect(statisticsWnd, SIGNAL(sigSegmentSelected(QString,int,int)), this, SLOT(onStatisticsSegmentSelected(QString,int,int)));

    // Create an internal QMainWindow to handle docking
    mainWindow = new QMainWindow(this);
    mainWindow->setWindowFlags(Qt::Widget);  // Make QMainWindow behave as a regular widget
    mainWindow->setDockNestingEnabled(true); // Allow nested docking if needed

    // Add QMainWindow to the layout
    mainLayout->addWidget(mainWindow);


    createVoltageSubWin();
    createCurrentSubWin();
    createConsumptionSubWin();

    epEnabledFlag = false;
    graphLoad = false;


    thread = new QThread(this);
    dataProcesingClass = new DataAnalyzerWorker();
    dataProcesingClass->moveToThread(thread);
    thread->setObjectName("OpenEPT - Data Analyzer");

    qRegisterMetaType<QVector<int> >("QVector<QVector<double>>");
    qRegisterMetaType<QVector<int> >("QVector<QPair<QString, int>>");

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

void DataAnalyzer::createVoltageSubWin() {
    // Create a dock widget
    QDockWidget *dockWidget = new QDockWidget("Voltage", this);
    dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);

    // Create a content widget with a layout for the dock widget
    QWidget *contentWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(2, 2, 2, 2);

    voltageChart             = new Plot(PLOT_MINIMUM_SIZE_WIDTH/2, PLOT_MINIMUM_SIZE_HEIGHT, false);
    voltageChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    voltageChart->setTitle("Voltage");
    voltageChart->setYLabel("[V]");
    voltageChart->setXLabel("[ms]");

    layout->addWidget(voltageChart);
    contentWidget->setLayout(layout);


    // Set the content widget in the dock widget
    dockWidget->setWidget(contentWidget);

    // Add the dock widget to the specified area in the main window
    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    // Make dock widgets floatable and closable
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
}

void DataAnalyzer::createCurrentSubWin()
{
    // Create a dock widget
    QDockWidget *dockWidget = new QDockWidget("Current", this);
    dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);

    // Create a content widget with a layout for the dock widget
    QWidget *contentWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(2, 2, 2, 2);


    currentChart             = new Plot(PLOT_MINIMUM_SIZE_WIDTH/2, PLOT_MINIMUM_SIZE_HEIGHT, false);
    currentChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    currentChart->setTitle("Current");
    currentChart->setYLabel("[mA]");
    currentChart->setXLabel("[ms]");

    layout->addWidget(currentChart);
    contentWidget->setLayout(layout);


    // Set the content widget in the dock widget
    dockWidget->setWidget(contentWidget);

    // Add the dock widget to the specified area in the main window
    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    // Make dock widgets floatable and closable
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
}

void DataAnalyzer::createConsumptionSubWin()
{
    // Create a dock widget
    QDockWidget *dockWidget = new QDockWidget("Consumption", this);
    dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);

    // Create a content widget with a layout for the dock widget
    QWidget *contentWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
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


    // Set the content widget in the dock widget
    dockWidget->setWidget(contentWidget);

    // Add the dock widget to the specified area in the main window
    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    // Make dock widgets floatable and closable
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
}




QStringList DataAnalyzer::listConsumptionProfiles()
{
    QStringList profiles;
    QDir dir(wsDirPath);

    if(!dir.exists()) return profiles;

    listConsumptionProfilesInDir(dir, "", profiles, 0);

    return profiles;
}

void DataAnalyzer::listConsumptionProfilesInDir(QDir dir, QString relativePath, QStringList& profiles, int depth)
{
    if(depth > 3) return;

    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach(const QFileInfo &entry, dir.entryInfoList())
    {
        QDir subDir(entry.filePath());
        QString subPath = relativePath.isEmpty() ? entry.fileName() : relativePath + "/" + entry.fileName();
        if(subDir.exists("OpenEPT.txt"))
        {
            profiles << subPath;
        }
        else
        {
            listConsumptionProfilesInDir(subDir, subPath, profiles, depth + 1);
        }
    }
}

void DataAnalyzer::realoadConsumptionProfiles()
{
    consumptionProfilesCB->clear();
    consumptionProfilesCB->setEnabled(false);
    consumptionProfilesName = listConsumptionProfiles();
    if(consumptionProfilesName.size() == 0)
    {
        detectedProfilesLabe->setText("No detected consumption profile");
    }
    else
    {
        consumptionProfilesCB->addItems(consumptionProfilesName);
        detectedProfilesLabe->setText("Select one consumption profile");
        consumptionProfilesCB->setEnabled(true);
    }
}

DataAnalyzer::~DataAnalyzer()
{
    statisticsThread->quit();
    statisticsThread->wait();
    delete statisticsWorker;
    delete statisticsWnd;
    delete ui;
}

void DataAnalyzer::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    realoadConsumptionProfiles();
}

void DataAnalyzer::setWorkspacePath(QString aWsDirPath)
{
    wsDirPath = aWsDirPath;
    realoadConsumptionProfiles();
}

void DataAnalyzer::onPlotXRangeChanged(QCPRange range)
{
    Plot *source = qobject_cast<Plot*>(sender());
    Plot *plots[3] = {voltageChart, currentChart, consumptionChart};

    for(int i = 0; i < 3; i++)
    {
        if(plots[i] == source) continue;
        plots[i]->setXRangeSynced(range);
    }
}

void DataAnalyzer::onGenerateStatistics()
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
    emit sigComputeStatistics(loadedVoltage, loadedVoltageKeys, loadedCurrent, loadedCurrentKeys, loadedMarkers);
}

void DataAnalyzer::onStatisticsFinished(QVector<dataanalyzer_segment_stat_t> stats, QVector<dataanalyzer_point_marker_t> points, dataanalyzer_segment_stat_t total, QStringList warnings)
{
    if(stats.isEmpty() && points.isEmpty() && warnings.isEmpty())
    {
        QMessageBox::information(this, "Statistics", "No \"<name> Start\" / \"<name> Stop\" marker pairs found");
        return;
    }
    statisticsWnd->setStatistics(selectedConsumptionProfile, stats, points, total, warnings);
    statisticsWnd->show();
    statisticsWnd->raise();
    statisticsWnd->activateWindow();
}

void DataAnalyzer::onStatisticsSegmentSelected(QString name, int startIndex, int endIndex)
{
    double min;
    double max;

    if(!graphLoad) return;
    if(startIndex < 0 || endIndex >= loadedVoltageKeys.size() || endIndex <= startIndex) return;

    min = loadedVoltageKeys[startIndex];
    max = loadedVoltageKeys[endIndex];
    voltageChart->zoomToKeyRange(min, max);
    currentChart->zoomToKeyRange(min, max);
    consumptionChart->zoomToKeyRange(min, max);

    raise();
    activateWindow();
}

void DataAnalyzer::onSaveAllPlots()
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
    if(!selectedConsumptionProfile.isEmpty())
    {
        defaultDir = wsDirPath + "/" + selectedConsumptionProfile;
    }

    dir = QFileDialog::getExistingDirectory(this, "Select folder for plot images", defaultDir);
    if(dir.isEmpty()) return;

    prefix = selectedConsumptionProfile.isEmpty() ? "profile" : selectedConsumptionProfile.section('/', -1).toLower().replace(' ', '_');

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

void DataAnalyzer::onDeleteConsumptionProfile()
{
    QString profile = consumptionProfilesCB->currentText();
    QString path;
    QDir dir;

    if(profile.isEmpty() || !consumptionProfilesCB->isEnabled())
    {
        QMessageBox::warning(this, "Delete profile", "No consumption profile selected");
        return;
    }

    path = QDir(wsDirPath).filePath(profile);
    dir = QDir(path);
    if(!dir.exists() || !dir.exists("OpenEPT.txt"))
    {
        QMessageBox::warning(this, "Delete profile", "Profile folder not found:\n" + path);
        return;
    }

    if(QMessageBox::question(this, "Delete profile",
                             "Delete consumption profile \"" + profile + "\"?\n\nFolder and all its files will be permanently removed:\n" + path,
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    if(graphLoad && selectedConsumptionProfile == profile)
    {
        voltageChart->clear();
        currentChart->clear();
        consumptionChart->clear();
        loadedMarkers.clear();
        graphLoad = false;
    }

    if(!dir.removeRecursively())
    {
        QMessageBox::warning(this, "Delete profile", "Unable to delete:\n" + path);
    }

    realoadConsumptionProfiles();
}

void DataAnalyzer::onRealoadConsumptionProfiles()
{
    realoadConsumptionProfiles();
}

void DataAnalyzer::onConsumptionProfileChanged(int index)
{
    selectedConsumptionProfile = consumptionProfilesCB->itemText(index);
}

void DataAnalyzer::loadConsumptionProfileData()
{
    QString summaryFilePath = wsDirPath + "/" + selectedConsumptionProfile + "/OpenEPT.txt";
    QVector<QPair<QString, QString>>    summaryInfo = parseSummaryFile(summaryFilePath);
    epEnabledFlag = false;

    QString epEnabled   = getValueForKey(summaryInfo, "EP Enabled");
    if(epEnabled == "1") epEnabledFlag = true;

    if(epEnabledFlag)
    {
        dataProcesingClass->setLimits(3);
    }
    else
    {
        dataProcesingClass->setLimits(2);
    }

    // Create a progress dialog without Cancel and Close buttons
    progressDialog = new QProgressDialog("Loading Data... ", "Cancel", 0, 100, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0);
    progressDialog->setAutoClose(true);

    // Remove Close (X) button and Cancel button
    progressDialog->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progressDialog->setWindowTitle("Load OpenEPT Data");

    progressDialog->show();

    emit processVolCurConRequest(wsDirPath, selectedConsumptionProfile);
}

QString DataAnalyzer::getValueForKey(const QVector<QPair<QString, QString>> &parsedData, const QString &key)
{
    for (const auto &pair : parsedData) {
        if (pair.first == key) {
            return pair.second;  // Return the corresponding value
        }
    }
    return "";  // Return empty string if key is not found
}

QVector<QPair<QString, QString>> DataAnalyzer::parseSummaryFile(const QString &filePath)
{
    QVector<QPair<QString, QString>> data;  // Store key-value pairs

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return data;
    }

    QTextStream in(&file);

    // Skip the first two lines (header)
    in.readLine();
    in.readLine();

    // Read and parse key-value pairs
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();  // Read and remove extra spaces
        if (line.isEmpty()) continue;  // Skip empty lines

        QStringList parts = line.split(":", Qt::SkipEmptyParts);  // Split at ':'
        if (parts.size() == 2) {
            QString key = parts[0].trimmed();   // Extract and trim key
            QString value = parts[1].trimmed(); // Extract and trim value

            data.append(qMakePair(key, value));
        } else {
            qWarning() << "Invalid format in line:" << line;
        }
    }

    file.close();
    return data;
}
void DataAnalyzer::onLoadConsumptionProfileData()
{
    loadConsumptionProfileData();

}

void DataAnalyzer::processingVolCurConDone(QVector<QVector<double> > vc, QVector<QVector<double> > cons)
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


    if(epEnabledFlag){
        emit processEPRequest(wsDirPath, selectedConsumptionProfile);
    }
    else{
        if (progressDialog) {
            progressDialog->close();
        }
    }
}

void DataAnalyzer::processingEPDone(QVector<QPair<QString, int> > epData)
{
    loadedMarkers = epData;
    consumptionChart->scatterAddGraph();
    consumptionChart->scatterAddAllDataWithName(epData);
    voltageChart->scatterAddGraph();
    voltageChart->scatterAddAllDataWithName(epData);
    currentChart->scatterAddGraph();
    currentChart->scatterAddAllDataWithName(epData);
    if (progressDialog) {
        progressDialog->close();
    }
}

void DataAnalyzer::updateProgress(int percentage)
{
    if (progressDialog) {
        progressDialog->setValue(percentage);
        if (percentage >= 100) {
            progressDialog->close();
        }
    }
}

void DataAnalyzer::updateProgressText(QString text)
{
    if (progressDialog) {
        progressDialog->setLabelText(text);
    }
}


DataAnalyzerWorker::DataAnalyzerWorker(QObject *parent) : QObject(parent)
{
    vcDataProcessingStartPercentage = 0;
    consumptionDataProcessingStartPercentage = 0;
    epDataProcessingStartPercentage = 0;


}

bool DataAnalyzerWorker::setLimits(int limitsNumber)
{
    if(limitsNumber == 2 )
    {
        vcDataProcessingStartPercentage = 10;
        consumptionDataProcessingStartPercentage = 55;
        epDataProcessingStartPercentage = 0;
        range = 45;
        return true;
    }
    if(limitsNumber == 3 )
    {

        vcDataProcessingStartPercentage = 10;
        consumptionDataProcessingStartPercentage = 40;
        epDataProcessingStartPercentage = 70;
        range = 30;
        return true;
    }
}

void DataAnalyzerWorker::processVoltCurConData(const QString &wsDirPath, const QString &selectedConsumptionProfile)
{
    // Simulating heavy processing
    qDebug() << "Processing started in thread:" << QThread::currentThread();

    QString voltageCurrentPath = wsDirPath + "/" + selectedConsumptionProfile + "/vc.csv";
    QString consPath = wsDirPath + "/" + selectedConsumptionProfile + "/cons.csv";
    QString epPath = wsDirPath + "/" + selectedConsumptionProfile + "/ep.csv";

    emit progressUpdated(vcDataProcessingStartPercentage);
    emit updateProgressText("Load Voltage and Current Data");
    QVector<QVector<double> >           vcData = parseVCData(voltageCurrentPath);

    emit progressUpdated(consumptionDataProcessingStartPercentage);
    emit updateProgressText("Load Consumption Data");

    QVector<QVector<double> >           consData = parseConsumptionData(consPath);

    // Emit a signal when processing is complete
    emit processingVolCurConFinished(vcData, consData);
}

void DataAnalyzerWorker::processEPData(const QString &wsDirPath, const QString &selectedConsumptionProfile)
{
    QString epPath = wsDirPath + "/" + selectedConsumptionProfile + "/ep.csv";
    emit progressUpdated(epDataProcessingStartPercentage);
    emit updateProgressText("Load Energy Points");
    QVector<QPair<QString, int>> epData = parseEPFile(epPath);
    emit processingEPFinished(epData);
}

#define DATAANALYZER_LINE_BUFFER    512

static const char* prvDATAANALYZER_ParseDouble(const char* p, double* value)
{
    double result = 0;
    double scale = 1;
    int sign = 1;
    int digits = 0;

    while(*p == ' ' || *p == '\t') p++;
    if(*p == '-') { sign = -1; p++; }
    else if(*p == '+') p++;
    while(*p >= '0' && *p <= '9')
    {
        result = result * 10.0 + (double)(*p - '0');
        p++;
        digits++;
    }
    if(*p == '.')
    {
        p++;
        while(*p >= '0' && *p <= '9')
        {
            result = result * 10.0 + (double)(*p - '0');
            scale *= 10.0;
            p++;
            digits++;
        }
    }
    if(digits == 0) return NULL;
    result = sign * result / scale;
    if(*p == 'e' || *p == 'E')
    {
        int expSign = 1;
        int exponent = 0;
        int expDigits = 0;
        p++;
        if(*p == '-') { expSign = -1; p++; }
        else if(*p == '+') p++;
        while(*p >= '0' && *p <= '9')
        {
            exponent = exponent * 10 + (*p - '0');
            p++;
            expDigits++;
        }
        if(expDigits == 0) return NULL;
        result *= pow(10.0, expSign * exponent);
    }
    while(*p == ' ' || *p == '\t') p++;
    *value = result;
    return p;
}

static bool prvDATAANALYZER_ParseLine(const char* line, int count, double* values)
{
    const char* p = line;

    for(int i = 0; i < count; i++)
    {
        p = prvDATAANALYZER_ParseDouble(p, &values[i]);
        if(p == NULL) return false;
        if(i < count - 1)
        {
            if(*p != ',') return false;
            p++;
        }
    }
    return (*p == 0 || *p == '\r' || *p == '\n');
}

static bool prvDATAANALYZER_LineComplete(const char* line, qint64 length)
{
    return (length > 0 && line[length - 1] == '\n');
}

QVector<QVector<double> > DataAnalyzerWorker::parseVCData(const QString &filePath)
{
    QVector<QVector<double>> data(4);
    char line[DATAANALYZER_LINE_BUFFER];
    double values[4];
    qint64 limit;
    qint64 length;
    int lastPercent = -1;
    int skipped = 0;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open file:" << filePath;
        return data;
    }

    limit = file.size();
    for(int i = 0; i < 4; i++) data[i].reserve((int)(limit / 32) + 16);

    file.readLine(line, sizeof(line));
    file.readLine(line, sizeof(line));

    while(file.pos() < limit)
    {
        length = file.readLine(line, sizeof(line));
        if(length <= 0) break;
        if(!prvDATAANALYZER_LineComplete(line, length)) break;
        if(prvDATAANALYZER_ParseLine(line, 4, values))
        {
            data[0].append(values[0]);
            data[1].append(values[1]);
            data[2].append(values[2]);
            data[3].append(values[3]);
        }
        else
        {
            skipped++;
        }
        int percent = (int)((file.pos() * (qint64)range) / limit);
        if(percent != lastPercent)
        {
            lastPercent = percent;
            emit progressUpdated(vcDataProcessingStartPercentage + percent);
        }
    }
    if(skipped > 0) qWarning() << "vc.csv: skipped" << skipped << "malformed lines";

    file.close();
    return data;
}

 QVector<QVector<double>> DataAnalyzerWorker::parseConsumptionData(const QString &filePath)
{
    QVector<QVector<double>> data(2);
    char line[DATAANALYZER_LINE_BUFFER];
    double values[2];
    qint64 limit;
    qint64 length;
    int lastPercent = -1;
    int skipped = 0;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open file:" << filePath;
        return data;
    }

    limit = file.size();
    for(int i = 0; i < 2; i++) data[i].reserve((int)(limit / 16) + 16);

    file.readLine(line, sizeof(line));
    file.readLine(line, sizeof(line));

    while(file.pos() < limit)
    {
        length = file.readLine(line, sizeof(line));
        if(length <= 0) break;
        if(!prvDATAANALYZER_LineComplete(line, length)) break;
        if(prvDATAANALYZER_ParseLine(line, 2, values))
        {
            data[0].append(values[0]);
            data[1].append(values[1]);
        }
        else
        {
            skipped++;
        }
        int percent = (int)((file.pos() * (qint64)range) / limit);
        if(percent != lastPercent)
        {
            lastPercent = percent;
            emit progressUpdated(consumptionDataProcessingStartPercentage + percent);
        }
    }
    if(skipped > 0) qWarning() << "cons.csv: skipped" << skipped << "malformed lines";

    file.close();
    return data;
}

QVector<QPair<QString, int> > DataAnalyzerWorker::parseEPFile(const QString &filePath)
{
    QVector<QPair<QString, int>> data;
    qint64 limit;
    int lastPercent = -1;
    int skipped = 0;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open file:" << filePath;
        return data;
    }

    limit = file.size();
    file.readLine();
    file.readLine();

    while(file.pos() < limit)
    {
        QByteArray line = file.readLine();
        if(line.isEmpty()) break;
        if(!line.endsWith('\n')) break;
        int comma = line.lastIndexOf(',');
        if(comma > 0)
        {
            bool ok;
            int key = line.mid(comma + 1).trimmed().toInt(&ok);
            if(ok)
            {
                data.append(qMakePair(QString::fromUtf8(line.left(comma)).trimmed(), key));
            }
            else
            {
                skipped++;
            }
        }
        else
        {
            skipped++;
        }
        int percent = (int)((file.pos() * (qint64)range) / limit);
        if(percent != lastPercent)
        {
            lastPercent = percent;
            emit progressUpdated(epDataProcessingStartPercentage + percent);
        }
    }
    if(skipped > 0) qWarning() << "ep.csv: skipped" << skipped << "malformed lines";

    file.close();
    return data;
}

