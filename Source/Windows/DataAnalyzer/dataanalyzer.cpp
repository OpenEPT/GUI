#include "dataanalyzer.h"
#include "ui_dataanalyzer.h"


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

    // Add the button and line edit to the horizontal layout
    topLayout->addWidget(detectedProfilesLabe);
    topLayout->addWidget(consumptionProfilesCB);
    topLayout->addWidget(reloadProfileNamesPushb);
    topLayout->addStretch();
    topLayout->addWidget(processFilePushb);

     mainLayout->addLayout(topLayout);

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

    consumptionChart             = new Plot(PLOT_MINIMUM_SIZE_WIDTH/2, PLOT_MINIMUM_SIZE_HEIGHT, false);
    consumptionChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    consumptionChart->setTitle("Consumption");
    consumptionChart->setYLabel("[mA]");
    consumptionChart->setXLabel("[ms]");

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
    QStringList subdirectories;

    QDir dir(wsDirPath);

    // Check if the main directory exists
    if (!dir.exists()) return subdirectories;

    // Set the filter to only look for directories, excluding "." and ".."
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    // Iterate through the directory entries and add the directory names to the list
    foreach (const QFileInfo &entry, dir.entryInfoList()) {
        if (entry.isDir()) {
            // Create a QDir object for the subdirectory
            QDir subDir(entry.filePath());
            // Check if OpenEPT.txt exists in the subdirectory
            if (subDir.exists("OpenEPT.txt")) {
                subdirectories << entry.fileName(); // Add the subdirectory name to the list
            }
        }
    }

    return subdirectories;
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
    delete ui;
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

QVector<QVector<double> > DataAnalyzerWorker::parseVCData(const QString &filePath)
{
    // Create a QVector to store each column
    QVector<QVector<double>> data(4);  // Initialize with 4 QVectors

    // Open the CSV file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return data;
    }

    QTextStream in(&file);

    // Skip the first two lines
    in.readLine();
    in.readLine();

    int totalLines = 0;
    while (!in.atEnd()) {
        in.readLine();
        totalLines++;
    }

    file.seek(0);  // Reset file pointer
    in.readLine();  // Skip header
    in.readLine();


    int processedLines = 0;
    // Read each line and parse the data
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(',');

        // Ensure we have exactly 4 columns in the line
        if (values.size() == 4) {
            bool ok;

            // Parse and add each column to the corresponding QVector
            double voltage = values[0].toDouble(&ok);
            if (ok) data[0].append(voltage);

            double time1 = values[1].toDouble(&ok);
            if (ok) data[1].append(time1);

            double current = values[2].toDouble(&ok);
            if (ok) data[2].append(current);

            double time2 = values[3].toDouble(&ok);
            if (ok) data[3].append(time2);
        } else {
            qWarning() << "Unexpected number of columns in line:" << line;
        }
        processedLines++;
        int percentage = (processedLines * range) / totalLines;  // Map to 0-40% range
        emit progressUpdated(vcDataProcessingStartPercentage + percentage);  // Start from 10%
    }

    file.close();
    return data;
}

 QVector<QVector<double>> DataAnalyzerWorker::parseConsumptionData(const QString &filePath)
{
    // Create a QVector to store each column
    QVector<QVector<double>> data(2);  // Initialize with 4 QVectors

    // Open the CSV file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return data;
    }

    QTextStream in(&file);

    // Skip the first two lines
    in.readLine();
    in.readLine();

    int totalLines = 0;
    while (!in.atEnd()) {
        in.readLine();
        totalLines++;
    }

    file.seek(0);  // Reset file pointer
    in.readLine();  // Skip header
    in.readLine();


    int processedLines = 0;

    // Read each line and parse the data
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(',');

        // Ensure we have exactly 4 columns in the line
        if (values.size() == 2) {
            bool ok;

            // Parse and add each column to the corresponding QVector
            double voltage = values[0].toDouble(&ok);
            if (ok) data[0].append(voltage);

            double time1 = values[1].toDouble(&ok);
            if (ok) data[1].append(time1);

        } else {
            qWarning() << "Unexpected number of columns in line:" << line;
        }
        processedLines++;
        int percentage = (processedLines * range) / totalLines;  // Map to 0-40% range
        emit progressUpdated(consumptionDataProcessingStartPercentage + percentage);  // Start from 10%    }
    }
    file.close();
    return data;
}

QVector<QPair<QString, int> > DataAnalyzerWorker::parseEPFile(const QString &filePath)
{
    // QVector to store name (QString) and key (int)
    QVector<QPair<QString, int>> data;

    // Open the CSV file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return data;
    }

    QTextStream in(&file);

    // Skip the first two lines
    in.readLine();
    in.readLine();

    int totalLines = 0;
    while (!in.atEnd()) {
        in.readLine();
        totalLines++;
    }

    file.seek(0);  // Reset file pointer
    in.readLine();  // Skip header
    in.readLine();


    int processedLines = 0;

    // Read and parse the data line by line
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(',');

        // Ensure there are exactly two columns
        if (values.size() == 2) {
            QString name = values[0].trimmed(); // Trim any whitespace
            bool ok;
            int key = values[1].toInt(&ok);

            if (ok) {
                data.append(qMakePair(name, key));
            } else {
                qWarning() << "Invalid key value in line:" << line;
            }
        } else {
            qWarning() << "Unexpected number of columns in line:" << line;
        }
        processedLines++;
        int percentage = (processedLines * range) / totalLines;  // Map to 0-40% range
        emit progressUpdated(epDataProcessingStartPercentage + percentage);  // Start from 10%    }
    }

    file.close();
    return data;
}

