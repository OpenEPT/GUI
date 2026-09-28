#ifndef PLOT_H
#define PLOT_H

#include <QWidget>
#include <QFont>
#include <QPushButton>
#include <QPair>
#include "Chart/qcustomplot.h"

class Plot : public QWidget
{
    Q_OBJECT
public:
    explicit    Plot(int mw, int mh, bool aEnableTracking = true, QWidget *parent = nullptr);
    void        scatterAddGraph();
    void        scatterAddData(QVector<double> data, QVector<double> keys);
    void        scatterAddAllDataWithName(QVector<QPair<QString, int>> data);
    void        scatterClearMarkers();
    void        scatterAddDataWithName(double value, double keys, QString name);
    void        scatterReplotDataWithName();
    void        setData(QVector<double> data, QVector<double> keys);
    void        overlaySetData(QVector<double> data, QVector<double> keys, QColor color = Qt::red);
    void        overlayClear();
    void        markerAddAtKey(double key, double value, QString name, QColor color);
    void        markersAtKeyClear();
    void        applyStyle(QString fontFamily, int labelFontSize, int tickFontSize,
                           int lineWidth, bool gridVisible, bool minorGridVisible);
    bool        saveImageToSvg(QString path);
    void        appendData(QVector<double> data, QVector<double> keys);
    void        setYRange(double min, double max);
    void        setYLabel(QString label);
    void        setXRange(double min, double max);
    void        setXRangeSynced(QCPRange range);
    void        zoomToKeyRange(double min, double max);
    bool        isAxisLocked();
    void        setXLabel(QString label);
    void        setTitle(QString aTitle);
    QString     getTitle();
    bool        saveImageToFile(QString path);
    void        clear();

signals:

    void        sigScatterKeyAndName(QString name, double key);
    void        sigXRangeChanged(QCPRange range);
    void        sigAxisLockChanged(bool locked);

private slots:
    void        onZoomIn();
    void        onZoomOut();
    void        onZoomExpand();
    void        onZoomArea();
    void        onMoveGraph();
    void        onTrackGraph();
    void        onSaveImage();
    void        onLockAxis();
    void        onXRangeChanged(const QCPRange &range);

private:
    QCustomPlot *plot;

    QPushButton *zoomIn;
    QPushButton *zoomOut;
    QPushButton *zoomExpand;
    QPushButton *zoomArea;
    QPushButton *moveGraph;
    QPushButton *lockAxis;
    QPushButton *saveImage;
    QPushButton *trackGraph;

    QFont       *scatterFont;

    QCPTextElement *title;

    QVector<double> xData;
    QVector<double> yData;
    QVector<double> plotXData;
    QVector<double> plotYData;
    QVector<double> epDataKey;
    QVector<QString> epDataName;
    QVector<QCPItemText *> textData;
    QCPGraph    *overlayGraph;
    QCPGraph    *keyMarkerGraph;
    QVector<QCPItemText *> keyMarkerText;

    bool        enableTracking;
    bool        replotActive;
    bool        scatterGraphAdded;
    bool        axisLocked;
    bool        xRangeSyncInProgress;

    void        setButtonStyle();
    void        rescaleYWithMarkers();
    void        trimMarkers(double minKey);
    void        showAllMarkers();
    QCPItemText* createMarkerLabel(double x, double y, QString name);
    bool        saveImageAsPng(QString path);
    bool        saveImageAsSvg(QString path);

};

#endif // PLOT_H
