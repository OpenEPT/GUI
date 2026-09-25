#include "dataanalyzer.h"
#include "ui_dataanalyzer.h"
#include <QFileDialog>
#include <math.h>
#include <QMessageBox>
#include <QDateTime>
#include <QPushButton>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QTabBar>

DataAnalyzer::DataAnalyzer(QWidget *parent, QString aWsDirPath) :
    QWidget(parent),
    ui(new Ui::DataAnalyzer)
{
    ui->setupUi(this);

    wsDirPath = aWsDirPath;

    QFont defaultFont("Arial", 10);
    setFont(defaultFont);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
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

    reloadProfileNamesPushb->setIcon(QIcon(QPixmap(":/images/NewSet/reload.png")));
    reloadProfileNamesPushb->setIconSize(QSize(30,30));
    reloadProfileNamesPushb->setToolTip("Reload consumption profiles");
    reloadProfileNamesPushb->setFixedSize(30, 30);
    connect(reloadProfileNamesPushb, SIGNAL(clicked(bool)), this, SLOT(onRealoadConsumptionProfiles()));

    processFilePushb->setIcon(QIcon(QPixmap(":/images/NewSet/load.png")));
    processFilePushb->setIconSize(QSize(30,30));
    processFilePushb->setToolTip("Load selected consumption profile in a new tab");
    processFilePushb->setFixedSize(30, 30);
    connect(processFilePushb, SIGNAL(clicked(bool)), this, SLOT(onLoadConsumptionProfileData()));

    deleteProfilePushb->setIcon(QIcon(QPixmap(":/images/NewSet/delete.png")));
    deleteProfilePushb->setIconSize(QSize(30,30));
    deleteProfilePushb->setToolTip("Delete selected consumption profile from disk");
    deleteProfilePushb->setFixedSize(30, 30);
    connect(deleteProfilePushb, SIGNAL(clicked(bool)), this, SLOT(onDeleteConsumptionProfile()));

    topLayout->addWidget(detectedProfilesLabe);
    topLayout->addWidget(reloadProfileNamesPushb);
    topLayout->addWidget(consumptionProfilesCB);
    topLayout->addWidget(processFilePushb);
    topLayout->addWidget(deleteProfilePushb);
    topLayout->addStretch();

    mainLayout->addLayout(topLayout);

    qRegisterMetaType<QVector<dataanalyzer_segment_stat_t>>("QVector<dataanalyzer_segment_stat_t>");
    qRegisterMetaType<QVector<QPair<QString, int>>>("QVector<QPair<QString, int>>");
    qRegisterMetaType<dataanalyzer_segment_stat_t>("dataanalyzer_segment_stat_t");
    qRegisterMetaType<QVector<dataanalyzer_point_marker_t>>("QVector<dataanalyzer_point_marker_t>");
    qRegisterMetaType<QVector<int> >("QVector<QVector<double>>");
    qRegisterMetaType<QVector<int> >("QVector<QPair<QString, int>>");

    mdiArea = new QMdiArea(this);
    mdiArea->setViewMode(QMdiArea::TabbedView);
    mdiArea->setTabsClosable(true);
    mdiArea->setTabsMovable(true);
    mdiArea->setDocumentMode(true);
    mdiArea->setTabPosition(QTabWidget::North);
    mainLayout->addWidget(mdiArea);

    maximizeProfilePushb = new QToolButton(mdiArea);
    maximizeProfilePushb->setIcon(QIcon(QPixmap(":/images/NewSet/expand.png")));
    maximizeProfilePushb->setIconSize(QSize(20,20));
    maximizeProfilePushb->setToolTip("Maximize selected profile in a separate window");
    maximizeProfilePushb->setAutoRaise(true);
    maximizeProfilePushb->setFixedSize(24, 24);
    maximizeProfilePushb->hide();
    connect(maximizeProfilePushb, SIGNAL(clicked(bool)), this, SLOT(onMaximizeProfile()));

    mdiArea->installEventFilter(this);
}

DataAnalyzer::~DataAnalyzer()
{
    for(int i = profiles.size() - 1; i >= 0; i--)
    {
        if(subWindowFor(profiles[i]) != NULL) continue;
        delete profiles[i];
    }

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

QStringList DataAnalyzer::listConsumptionProfiles()
{
    QStringList profileList;
    QDir dir(wsDirPath);

    if(!dir.exists()) return profileList;

    listConsumptionProfilesInDir(dir, "", profileList, 0);

    return profileList;
}

void DataAnalyzer::listConsumptionProfilesInDir(QDir dir, QString relativePath, QStringList& profileList, int depth)
{
    if(depth > 3) return;

    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach(const QFileInfo &entry, dir.entryInfoList())
    {
        QDir subDir(entry.filePath());
        QString subPath = relativePath.isEmpty() ? entry.fileName() : relativePath + "/" + entry.fileName();
        if(subDir.exists("OpenEPT.txt"))
        {
            profileList << subPath;
        }
        else
        {
            listConsumptionProfilesInDir(subDir, subPath, profileList, depth + 1);
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

void DataAnalyzer::onRealoadConsumptionProfiles()
{
    realoadConsumptionProfiles();
}

void DataAnalyzer::onConsumptionProfileChanged(int index)
{
    selectedConsumptionProfile = consumptionProfilesCB->itemText(index);
}

void DataAnalyzer::attachProfile(DataAnalyzerProfile *profile, QString title)
{
    QMdiSubWindow *subWindow = mdiArea->addSubWindow(profile);

    subWindow->setWindowTitle(title);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->showMaximized();
    mdiArea->setActiveSubWindow(subWindow);
    profile->setDetached(false);
    profile->show();
    updateMaximizeButton();
}

void DataAnalyzer::onLoadConsumptionProfileData()
{
    DataAnalyzerProfile *profile;
    QString title;
    int counter;

    if(selectedConsumptionProfile.isEmpty() || !consumptionProfilesCB->isEnabled())
    {
        QMessageBox::warning(this, "Load profile", "No consumption profile selected");
        return;
    }

    counter = profileCounters.value(selectedConsumptionProfile, 0) + 1;
    profileCounters[selectedConsumptionProfile] = counter;
    title = selectedConsumptionProfile.section('/', -1);
    if(counter > 1) title += " (" + QString::number(counter) + ")";

    profile = new DataAnalyzerProfile(wsDirPath, selectedConsumptionProfile);
    profiles.append(profile);

    connect(profile, SIGNAL(sigDockStateToggleRequested()), this, SLOT(onProfileDockStateToggleRequested()));
    connect(profile, SIGNAL(destroyed(QObject*)), this, SLOT(onProfileDestroyed(QObject*)));

    attachProfile(profile, title);

    profile->startLoading();
}

QMdiSubWindow* DataAnalyzer::subWindowFor(DataAnalyzerProfile *profile)
{
    QList<QMdiSubWindow*> subWindows = mdiArea->subWindowList();

    for(int i = 0; i < subWindows.size(); i++)
    {
        if(subWindows[i]->widget() == profile) return subWindows[i];
    }

    return NULL;
}

bool DataAnalyzer::eventFilter(QObject *object, QEvent *event)
{
    if(object == mdiArea && (event->type() == QEvent::Resize ||
                             event->type() == QEvent::Show ||
                             event->type() == QEvent::LayoutRequest))
    {
        updateMaximizeButton();
    }

    return QWidget::eventFilter(object, event);
}

void DataAnalyzer::updateMaximizeButton()
{
    QTabBar *tabBar = mdiArea->findChild<QTabBar*>();

    if(tabBar == NULL || mdiArea->subWindowList().isEmpty())
    {
        maximizeProfilePushb->hide();
        return;
    }

    tabBar->setMaximumWidth(mdiArea->width() - maximizeProfilePushb->width() - 4);

    maximizeProfilePushb->move(mdiArea->width() - maximizeProfilePushb->width() - 2,
                               (tabBar->height() - maximizeProfilePushb->height()) / 2);
    maximizeProfilePushb->show();
    maximizeProfilePushb->raise();
}

void DataAnalyzer::onMaximizeProfile()
{
    QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow();

    if(activeSubWindow == NULL) return;

    toggleProfileDockState(qobject_cast<DataAnalyzerProfile*>(activeSubWindow->widget()));
}

void DataAnalyzer::onProfileDockStateToggleRequested()
{
    toggleProfileDockState(qobject_cast<DataAnalyzerProfile*>(sender()));
}

void DataAnalyzer::toggleProfileDockState(DataAnalyzerProfile *profile)
{
    QMdiSubWindow *subWindow;

    if(profile == NULL) return;

    subWindow = subWindowFor(profile);

    if(subWindow != NULL)
    {
        QString title = subWindow->windowTitle();

        subWindow->setWidget(NULL);
        profile->setParent(NULL);
        mdiArea->removeSubWindow(subWindow);
        subWindow->deleteLater();

        profile->setWindowTitle("Data Analyzer - " + title);
        profile->setWindowFlags(Qt::Window);
        profile->setAttribute(Qt::WA_DeleteOnClose);
        profile->setDetached(true);
        profile->resize(1000, 700);
        profile->show();
        profile->raise();
        profile->activateWindow();
        updateMaximizeButton();
        return;
    }

    profile->setAttribute(Qt::WA_DeleteOnClose, false);
    profile->setWindowFlags(Qt::Widget);
    attachProfile(profile, profile->windowTitle().section(" - ", -1));
    updateMaximizeButton();
}

void DataAnalyzer::onProfileDestroyed(QObject *object)
{
    for(int i = 0; i < profiles.size(); i++)
    {
        if(profiles[i] == object)
        {
            profiles.remove(i);
            return;
        }
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

    for(int i = profiles.size() - 1; i >= 0; i--)
    {
        if(profiles[i]->getProfileName() != profile) continue;

        QMdiSubWindow *subWindow = subWindowFor(profiles[i]);
        if(subWindow != NULL) subWindow->close();
        else profiles[i]->close();
    }

    if(!dir.removeRecursively())
    {
        QMessageBox::warning(this, "Delete profile", "Unable to delete:\n" + path);
    }

    realoadConsumptionProfiles();
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

