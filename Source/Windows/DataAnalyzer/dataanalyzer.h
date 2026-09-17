#ifndef DATAANALYZER_H
#define DATAANALYZER_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QComboBox>
#include <QMdiArea>
#include <QBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QMdiSubWindow>
#include <QDockWidget>
#include <QToolBar>
#include "dataanalyzerstatistics.h"
#include <QDir>
#include <QShowEvent>
#include <QMainWindow>
#include <QProgressBar>
#include <QFuture>
#include <QtConcurrent>
#include <QThread>
#include <QPair>
#include "Windows/Plot/plot.h"

namespace Ui {
class DataAnalyzer;
}

class DataAnalyzerWorker : public QObject
{
    Q_OBJECT

public:
    explicit DataAnalyzerWorker(QObject *parent = nullptr);
    bool     setLimits(int limitsNumber);

public slots:
    void processVoltCurConData(const QString &wsDirPath, const QString &selectedConsumptionProfile);
    void processEPData(const QString &wsDirPath, const QString &selectedConsumptionProfile);

signals:
    void processingVolCurConFinished(QVector<QVector<double> > voltage, QVector<QVector<double>> current);
    void processingEPFinished(QVector<QPair<QString, int>> epata);
    void progressUpdated(int percentage);
    void updateProgressText(QString text);

private:
    QVector<QVector<double>>            parseVCData(const QString &filePath);
     QVector<QVector<double>>           parseConsumptionData(const QString &filePath);
    QVector<QPair<QString, int>>        parseEPFile(const QString &filePath);

    int     vcDataProcessingStartPercentage;
    int     consumptionDataProcessingStartPercentage;
    int     epDataProcessingStartPercentage;
    double  range;

};


class DataAnalyzer : public QWidget
{
    Q_OBJECT

public:
    void                                setWorkspacePath(QString aWsDirPath);

protected:
    void                                showEvent(QShowEvent *event) override;

public:
    explicit DataAnalyzer(QWidget *parent = nullptr, QString aWsDirPath="");
    void     loadConsumptionProfileData();
    ~DataAnalyzer();

public slots:
    void                                onDeleteConsumptionProfile();
    void                                onGenerateStatistics();
    void                                onStatisticsSegmentSelected(QString name, int startIndex, int endIndex);
    void                                onStatisticsFinished(QVector<dataanalyzer_segment_stat_t> stats, dataanalyzer_segment_stat_t total, QStringList warnings);
    void                                onSaveAllPlots();
    void                                onPlotXRangeChanged(QCPRange range);
    void    onRealoadConsumptionProfiles();
    void    onConsumptionProfileChanged(int index);
    void    onLoadConsumptionProfileData();
    void    updateProgress(int percentage);
    void    updateProgressText(QString text);
    void    processingVolCurConDone(QVector<QVector<double> > vc, QVector<QVector<double>> cons);
    void    processingEPDone(QVector<QPair<QString, int>> epData);
signals:
    void                                sigComputeStatistics(QVector<double> voltage, QVector<double> voltageKeys, QVector<double> current, QVector<double> currentKeys, QVector<QPair<QString, int>> markers);
    void    processVolCurConRequest(const QString &filePath, const QString &selectedConsumptionProfile);
    void    processEPRequest(const QString &filePath, const QString &selectedConsumptionProfile);

private:
    Ui::DataAnalyzer                    *ui;
    QMainWindow                         *mainWindow;
    QToolBar                            *plotsToolBar;
    QThread                             *statisticsThread;
    DataAnalyzerStatisticsWorker        *statisticsWorker;
    DataAnalyzerStatisticsWnd           *statisticsWnd;
    QVector<double>                     loadedVoltage;
    QVector<double>                     loadedVoltageKeys;
    QVector<double>                     loadedCurrent;
    QVector<double>                     loadedCurrentKeys;
    QVector<QPair<QString, int>>        loadedMarkers;

    QLabel*                             detectedProfilesLabe;

    QString                             wsDirPath;

    Plot                                *voltageChart;
    Plot                                *currentChart;
    Plot                                *consumptionChart;
    bool                                epEnabledFlag;
    bool                                graphLoad;

    QProgressDialog                     *progressDialog;


    QString                             selectedConsumptionProfile;

    QThread                             *thread;
    DataAnalyzerWorker                  *dataProcesingClass;

    void                                createVoltageSubWin();
    void                                createCurrentSubWin();
    void                                createConsumptionSubWin();

    QComboBox                           *consumptionProfilesCB;
    QStringList                         consumptionProfilesName;

    QVector<QPair<QString, QString>>    parseSummaryFile(const QString &filePath);
    QStringList                         listConsumptionProfiles();
    QString                             getValueForKey(const QVector<QPair<QString, QString> > &parsedData, const QString &key);


    void                                realoadConsumptionProfiles();
    void                                listConsumptionProfilesInDir(QDir dir, QString relativePath, QStringList& profiles, int depth);
};

#endif // DATAANALYZER_H
