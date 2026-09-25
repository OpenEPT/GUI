#ifndef DATAANALYZERPROFILE_H
#define DATAANALYZERPROFILE_H

#include <QWidget>
#include <QString>
#include <QVector>
#include <QPair>
#include <QMainWindow>
#include <QToolBar>
#include <QThread>
#include <QProgressDialog>
#include <QAction>

#include "dataanalyzerstatistics.h"
#include "dataanalyzerworker.h"
#include "Windows/Plot/plot.h"

class DataAnalyzerProfile : public QWidget
{
    Q_OBJECT

public:
    explicit                            DataAnalyzerProfile(QString aWsDirPath, QString aProfileName, QWidget *parent = nullptr);
                                        ~DataAnalyzerProfile();

    QString                             getProfileName();
    void                                startLoading();
    void                                setDetached(bool detached);

signals:
    void                                sigDockStateToggleRequested();
    void                                sigCloseRequested();

    void                                processVolCurConRequest(const QString &filePath, const QString &selectedConsumptionProfile);
    void                                processEPRequest(const QString &filePath, const QString &selectedConsumptionProfile);
    void                                sigComputeStatistics(QVector<double> voltage, QVector<double> voltageKeys, QVector<double> current, QVector<double> currentKeys, QVector<QPair<QString, int>> markers);

public slots:
    void                                onGenerateStatistics();
    void                                onStatisticsProgress(int percentage, QString text);
    void                                onStatisticsFinished(QVector<dataanalyzer_segment_stat_t> stats, QVector<dataanalyzer_point_marker_t> points, dataanalyzer_segment_stat_t total, QStringList warnings);
    void                                onStatisticsSegmentSelected(QString name, int startIndex, int endIndex);
    void                                onStatisticsMarkerVisibilityChanged(QVector<dataanalyzer_segment_stat_t> segments, QVector<bool> expanded);
    void                                onSaveAllPlots();
    void                                onPlotXRangeChanged(QCPRange range);
    void                                onDetachClicked();
    void                                processingVolCurConDone(QVector<QVector<double> > vc, QVector<QVector<double>> cons);
    void                                processingEPDone(QVector<QPair<QString, int>> epData);
    void                                updateProgress(int percentage);
    void                                updateProgressText(QString text);

private:
    void                                createVoltageSubWin();
    void                                createCurrentSubWin();
    void                                createConsumptionSubWin();
    void                                applyMarkersToPlots(QVector<QPair<QString, int>> markers);
    QVector<QPair<QString, QString>>    parseSummaryFile(const QString &filePath);
    QString                             getValueForKey(const QVector<QPair<QString, QString> > &parsedData, const QString &key);

    QMainWindow                         *mainWindow;
    QToolBar                            *plotsToolBar;
    QAction                             *detachAction;

    Plot                                *voltageChart;
    Plot                                *currentChart;
    Plot                                *consumptionChart;

    QThread                             *statisticsThread;
    DataAnalyzerStatisticsWorker        *statisticsWorker;
    DataAnalyzerStatisticsWnd           *statisticsWnd;

    QThread                             *thread;
    DataAnalyzerWorker                  *dataProcesingClass;

    QVector<double>                     loadedVoltage;
    QVector<double>                     loadedVoltageKeys;
    QVector<double>                     loadedCurrent;
    QVector<double>                     loadedCurrentKeys;
    QVector<QPair<QString, int>>        loadedMarkers;

    QString                             wsDirPath;
    QString                             profileName;

    bool                                epEnabledFlag;
    bool                                graphLoad;

    QProgressDialog                     *progressDialog;
    QProgressDialog                     *statisticsProgressDialog;
};

#endif // DATAANALYZERPROFILE_H
