#ifndef BATTERYOCVANALYSISWND_H
#define BATTERYOCVANALYSISWND_H

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QVector>
#include <QWidget>

#include "Chart/qcustomplot.h"
#include "batteryparamsplotsettings.h"

#define BATTERYOCV_DEGREE_MIN_DEFAULT       2
#define BATTERYOCV_DEGREE_MAX_DEFAULT       20
#define BATTERYOCV_DEGREE_LIMIT             20
#define BATTERYOCV_NOISE_DEFAULT            5.0
#define BATTERYOCV_CURVE_POINTS             400
#define BATTERYOCV_SENSITIVITY_PERCENTILE   0.95
#define BATTERYOCV_SENSITIVITY_MARGIN       1.4

typedef struct
{
    int             degree;
    QVector<double> coefficients;
    double          rmse;
    double          meanError;
    double          standardDeviation;
    bool            valid;
}batteryocv_fit_t;

class BatteryOcvAnalysisWnd : public QWidget
{
    Q_OBJECT

public:
    explicit            BatteryOcvAnalysisWnd(QWidget *parent = nullptr);

    void                setData(QVector<double> soc, QVector<double> ocv);
    void                setPlotSettings(batteryparams_plot_settings_t settings);

private slots:
    void                onDegreeRangeChanged();
    void                onSelectionChanged();
    void                onNoiseChanged();
    void                onSaveImage();

private:
    QTabWidget         *tabWidget;
    QCustomPlot        *ocvPlot;
    QCustomPlot        *sensitivityPlot;
    QCustomPlot        *errorPlot;

    QLineEdit          *degreeMinEdit;
    QLineEdit          *degreeMaxEdit;
    QLineEdit          *noiseEdit;
    QTableWidget       *degreeTable;
    QCheckBox          *measuredCheckBox;
    QLabel             *summaryLabel;

    QVector<double>     measuredSoc;
    QVector<double>     measuredOcv;
    QVector<batteryocv_fit_t> fits;

    batteryparams_plot_settings_t plotSettings;
    double              socOffset;
    double              socScale;
    bool                tableUpdating;

    bool                polynomialFit(int degree, QVector<double> *coefficients);
    double              polynomialValue(const QVector<double> &coefficients, double soc);
    double              polynomialSlope(const QVector<double> &coefficients, double soc);
    QColor              degreeColor(int index);

    void                buildFits();
    void                updateOcvPlot();
    void                updateSensitivityPlot();
    void                updateErrorPlot();
    void                updateSummary();
    void                updateAll();
    double              noiseGet();
};

#endif // BATTERYOCVANALYSISWND_H
