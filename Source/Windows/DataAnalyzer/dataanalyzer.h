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
    explicit DataAnalyzer(QWidget *parent = nullptr, QString aWsDirPath="");
    void     loadConsumptionProfileData();
    ~DataAnalyzer();

public slots:
    void    onRealoadConsumptionProfiles();
    void    onConsumptionProfileChanged(int index);
    void    onLoadConsumptionProfileData();
    void    updateProgress(int percentage);
    void    updateProgressText(QString text);
    void    processingVolCurConDone(QVector<QVector<double> > vc, QVector<QVector<double>> cons);
    void    processingEPDone(QVector<QPair<QString, int>> epData);
signals:
    void    processVolCurConRequest(const QString &filePath, const QString &selectedConsumptionProfile);
    void    processEPRequest(const QString &filePath, const QString &selectedConsumptionProfile);

private:
    Ui::DataAnalyzer                    *ui;
    QMainWindow                         *mainWindow;

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
};

#endif // DATAANALYZER_H
