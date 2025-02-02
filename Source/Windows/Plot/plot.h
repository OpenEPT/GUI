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
    void        scatterAddDataWithName(double value, double keys, QString name);
    void        scatterReplotDataWithName();
    void        setData(QVector<double> data, QVector<double> keys);
    void        appendData(QVector<double> data, QVector<double> keys);
    void        setYRange(double min, double max);
    void        setYLabel(QString label);
    void        setXRange(double min, double max);
    void        setXLabel(QString label);
    void        setTitle(QString aTitle);
    void        clear();

signals:

    void        sigScatterKeyAndName(QString name, double key);

private slots:
    void        onZoomIn();
    void        onZoomOut();
    void        onZoomExpand();
    void        onZoomArea();
    void        onMoveGraph();
    void        onTrackGraph();

private:
    QCustomPlot *plot;

    QPushButton *zoomIn;
    QPushButton *zoomOut;
    QPushButton *zoomExpand;
    QPushButton *zoomArea;
    QPushButton *moveGraph;
    QPushButton *trackGraph;

    QFont       *scatterFont;

    QCPTextElement *title;

    QVector<double> xData;
    QVector<double> yData;
    QVector<double> epDataKey;
    QVector<QString> epDataName;
    QVector<QCPItemText *> textData;

    bool        enableTracking;
    bool        replotActive;
    bool        scatterGraphAdded;

    void        setButtonStyle();

};

#endif // PLOT_H
