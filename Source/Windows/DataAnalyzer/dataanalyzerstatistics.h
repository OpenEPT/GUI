#ifndef DATAANALYZERSTATISTICS_H
#define DATAANALYZERSTATISTICS_H

#include <QObject>
#include <QWidget>
#include <QThread>
#include <QVector>
#include <QPair>
#include <QString>
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QColor>
#include <QPaintEvent>
#include <QMouseEvent>

typedef struct
{
    QString         name;
    int             startIndex;
    int             endIndex;
    double          consumption;
    double          energy;
    double          duration;
    double          maxCurrent;
    double          minCurrent;
    double          avgCurrent;
    double          maxVoltage;
    double          minVoltage;
    double          avgVoltage;
    int             parentIndex;
}dataanalyzer_segment_stat_t;

class DataAnalyzerStatisticsWorker : public QObject
{
    Q_OBJECT
public:
    explicit                DataAnalyzerStatisticsWorker(QObject *parent = nullptr);

    static bool             markerIsStart(QString name, QString* segmentName);
    static bool             markerIsStop(QString name, QString* segmentName);

signals:
    void                    sigStatisticsFinished(QVector<dataanalyzer_segment_stat_t> stats, dataanalyzer_segment_stat_t total, QStringList warnings);

public slots:
    void                    onComputeStatistics(QVector<double> voltage, QVector<double> voltageKeys, QVector<double> current, QVector<double> currentKeys, QVector<QPair<QString, int>> markers);

private:
    bool                    computeSegment(QVector<double>& voltage, QVector<double>& keys, QVector<double>& current, dataanalyzer_segment_stat_t* stat);
};

typedef struct
{
    QString         name;
    double          share;
    QColor          color;
    int             parentIndex;
    int             startIndex;
    int             level;
    double          x0;
    double          x1;
}dataanalyzer_share_t;

class DataAnalyzerShareBar : public QWidget
{
    Q_OBJECT
public:
    explicit                DataAnalyzerShareBar(QWidget *parent = nullptr);

    void                    setShares(QVector<dataanalyzer_share_t> shares);

protected:
    void                    paintEvent(QPaintEvent *event) override;
    void                    mouseMoveEvent(QMouseEvent *event) override;

private:
    void                    layoutShares();
    void                    layoutChildren(int parent, double x0, double x1, double parentShare, int level);
    int                     shareAt(QPoint pos);

    QVector<dataanalyzer_share_t> shares;
    int                     levels;
};

class DataAnalyzerStatisticsItem : public QTreeWidgetItem
{
public:
    explicit                DataAnalyzerStatisticsItem(int segmentIndex);
    bool                    operator<(const QTreeWidgetItem &other) const override;
};

class DataAnalyzerStatisticsWnd : public QWidget
{
    Q_OBJECT
public:
    explicit                DataAnalyzerStatisticsWnd(QWidget *parent = nullptr);

    void                    setStatistics(QString profileName, QVector<dataanalyzer_segment_stat_t> stats, dataanalyzer_segment_stat_t total, QStringList warnings);

    static QString          formatConsumption(double mAh);
    static QString          formatEnergy(double mJ);
    static QString          formatDuration(double ms);
    static QString          formatClock(double ms);
    static bool             parseClock(QString text, double* ms);
    static QColor           segmentColor(int index);

signals:
    void                    sigSegmentSelected(QString name, int startIndex, int endIndex);

private slots:
    void                    onExportCsv();
    void                    onResetEdits();
    void                    onBatteryCapacityChanged();
    void                    onTargetTimeChanged();
    void                    onCycleReferenceChanged();
    void                    onItemChanged(QTreeWidgetItem *item, int column);
    void                    onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    void                    fillTable();
    void                    updateBatteryEstimate();
    void                    setCell(QTreeWidgetItem *item, int column, double value, QString text, bool editable);
    void                    refreshItemValues(int index);
    void                    applyDelta(int index, double consumptionDelta, double energyDelta, double durationDelta);
    int                     cycleReferenceIndex();
    void                    markedTotals(double* consumption, double* energy, double* duration);
    QString                 formatInfo(QString title, double duration, double consumption, double energy);
    double                  editedTotalConsumption();
    double                  editedCycleDuration();
    double                  editedTotalEnergy();

    QTreeWidget             *table;
    QVector<QTreeWidgetItem*> items;
    QPushButton             *exportButton;
    QPushButton             *resetButton;
    QLineEdit               *batteryCapacityEdit;
    QLineEdit               *targetTimeEdit;
    QComboBox               *cycleReferenceCombo;
    QLabel                  *requiredCapacityLabel;
    DataAnalyzerShareBar    *shareBar;
    QLabel                  *cycleDurationLabel;
    QLabel                  *cycleConsumptionLabel;
    QLabel                  *averageCurrentLabel;
    QLabel                  *markedInfoLabel;
    QLabel                  *unmarkedInfoLabel;
    QLabel                  *wholeInfoLabel;
    QLabel                  *cyclesLabel;
    QLabel                  *operatingTimeLabel;
    QString                 profileName;
    QVector<dataanalyzer_segment_stat_t> statistics;
    QVector<dataanalyzer_segment_stat_t> edited;
    dataanalyzer_segment_stat_t total;
    bool                    tableUpdating;
};

Q_DECLARE_METATYPE(dataanalyzer_segment_stat_t)

#endif // DATAANALYZERSTATISTICS_H
