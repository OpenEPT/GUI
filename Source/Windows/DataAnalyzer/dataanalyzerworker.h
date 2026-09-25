#ifndef DATAANALYZERWORKER_H
#define DATAANALYZERWORKER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QPair>

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
    QVector<QVector<double>>            parseConsumptionData(const QString &filePath);
    QVector<QPair<QString, int>>        parseEPFile(const QString &filePath);

    int     vcDataProcessingStartPercentage;
    int     consumptionDataProcessingStartPercentage;
    int     epDataProcessingStartPercentage;
    double  range;
};

#endif // DATAANALYZERWORKER_H
