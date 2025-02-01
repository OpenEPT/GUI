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


    QString                             selectedConsumptionProfile;

    void                                createVoltageSubWin();
    void                                createCurrentSubWin();
    void                                createConsumptionSubWin();

    QComboBox                           *consumptionProfilesCB;
    QStringList                         consumptionProfilesName;

    QVector<QVector<double>>            parseVCData(const QString &filePath);
    QVector<QVector<double>>            parseConsumptionData(const QString &filePath);
    QVector<QPair<QString, int>>        parseEPFile(const QString &filePath);
    QVector<QPair<QString, QString>>    parseSummaryFile(const QString &filePath);
    QStringList                         listConsumptionProfiles();
    QString                             getValueForKey(const QVector<QPair<QString, QString> > &parsedData, const QString &key);


    void                                realoadConsumptionProfiles();
};

#endif // DATAANALYZER_H
