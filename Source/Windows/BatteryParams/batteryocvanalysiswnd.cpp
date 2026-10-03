#include "batteryocvanalysiswnd.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <math.h>

#define BATTERYOCV_EDIT_WIDTH       70
#define BATTERYOCV_ROW_HEIGHT       26

static QTableWidgetItem* prvBATTERYOCV_ItemCreate(QString text)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);

    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);

    return item;
}

BatteryOcvAnalysisWnd::BatteryOcvAnalysisWnd(QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QVBoxLayout *controlLayout = new QVBoxLayout();
    QGroupBox *degreeGroup = new QGroupBox("Polynomial degrees", this);
    QVBoxLayout *degreeGroupLayout = new QVBoxLayout(degreeGroup);
    QGroupBox *noiseGroup = new QGroupBox("Noise sensitivity", this);
    QVBoxLayout *noiseGroupLayout = new QVBoxLayout(noiseGroup);
    QHBoxLayout *rangeLayout = new QHBoxLayout();
    QHBoxLayout *noiseLayout = new QHBoxLayout();
    QPushButton *saveButton = new QPushButton("Save image", this);
    QStringList degreeHeader;

    setFont(defaultFont);
    setWindowTitle("OCV analysis");

    tabWidget = new QTabWidget(this);

    ocvPlot = new QCustomPlot(this);
    ocvPlot->setInteraction(QCP::iRangeDrag, true);
    ocvPlot->setInteraction(QCP::iRangeZoom, true);
    ocvPlot->legend->setVisible(true);
    ocvPlot->legend->setFont(defaultFont);
    ocvPlot->xAxis->setLabel("SoC [%]");
    ocvPlot->yAxis->setLabel("OCV [V]");
    tabWidget->addTab(ocvPlot, "OCV(SoC)");

    sensitivityPlot = new QCustomPlot(this);
    sensitivityPlot->setInteraction(QCP::iRangeDrag, true);
    sensitivityPlot->setInteraction(QCP::iRangeZoom, true);
    sensitivityPlot->legend->setVisible(true);
    sensitivityPlot->legend->setFont(defaultFont);
    sensitivityPlot->xAxis->setLabel("SoC [%]");
    sensitivityPlot->yAxis->setLabel("dSoC/dV [%/V]");
    tabWidget->addTab(sensitivityPlot, "Sensitivity dSoC/dV");

    errorPlot = new QCustomPlot(this);
    errorPlot->setInteraction(QCP::iRangeDrag, true);
    errorPlot->setInteraction(QCP::iRangeZoom, true);
    errorPlot->legend->setVisible(true);
    errorPlot->legend->setFont(defaultFont);
    errorPlot->xAxis->setLabel("Polynomial degree");
    errorPlot->yAxis->setLabel("Fit error [mV]");
    tabWidget->addTab(errorPlot, "Fit error");

    degreeMinEdit = new QLineEdit(QString::number(BATTERYOCV_DEGREE_MIN_DEFAULT), this);
    degreeMinEdit->setFixedSize(BATTERYOCV_EDIT_WIDTH, BATTERYOCV_ROW_HEIGHT);
    degreeMaxEdit = new QLineEdit(QString::number(BATTERYOCV_DEGREE_MAX_DEFAULT), this);
    degreeMaxEdit->setFixedSize(BATTERYOCV_EDIT_WIDTH, BATTERYOCV_ROW_HEIGHT);

    rangeLayout->addWidget(new QLabel("From", this));
    rangeLayout->addWidget(degreeMinEdit);
    rangeLayout->addWidget(new QLabel("to", this));
    rangeLayout->addWidget(degreeMaxEdit);
    rangeLayout->addStretch();
    degreeGroupLayout->addLayout(rangeLayout);

    degreeHeader << "Draw" << "Degree" << "RMSE [mV]" << "Mean [mV]" << "Std [mV]";
    degreeTable = new QTableWidget(0, 5, this);
    degreeTable->setHorizontalHeaderLabels(degreeHeader);
    degreeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    degreeTable->verticalHeader()->setVisible(false);
    degreeTable->verticalHeader()->setDefaultSectionSize(20);
    degreeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    degreeTable->setMinimumHeight(320);
    degreeTable->setMinimumWidth(330);
    degreeTable->setToolTip("Fit error of every polynomial degree.\nTick a degree to draw it next to the measured curve");
    degreeGroupLayout->addWidget(degreeTable, 1);

    measuredCheckBox = new QCheckBox("Draw measured points", this);
    measuredCheckBox->setChecked(true);
    degreeGroupLayout->addWidget(measuredCheckBox);

    noiseEdit = new QLineEdit(QString::number(BATTERYOCV_NOISE_DEFAULT), this);
    noiseEdit->setFixedSize(BATTERYOCV_EDIT_WIDTH, BATTERYOCV_ROW_HEIGHT);
    noiseLayout->addWidget(new QLabel("Voltage noise [mV]", this));
    noiseLayout->addStretch();
    noiseLayout->addWidget(noiseEdit);
    noiseGroupLayout->addLayout(noiseLayout);

    summaryLabel = new QLabel(this);
    summaryLabel->setWordWrap(true);
    summaryLabel->setMinimumWidth(300);
    noiseGroupLayout->addWidget(summaryLabel);

    controlLayout->addWidget(degreeGroup, 1);
    controlLayout->addWidget(noiseGroup);
    controlLayout->addWidget(saveButton);

    mainLayout->addWidget(tabWidget, 1);
    mainLayout->addLayout(controlLayout);

    connect(degreeMinEdit, &QLineEdit::editingFinished, this, &BatteryOcvAnalysisWnd::onDegreeRangeChanged);
    connect(degreeMaxEdit, &QLineEdit::editingFinished, this, &BatteryOcvAnalysisWnd::onDegreeRangeChanged);
    connect(degreeTable, &QTableWidget::itemChanged, this, &BatteryOcvAnalysisWnd::onSelectionChanged);
    connect(measuredCheckBox, &QCheckBox::toggled, this, &BatteryOcvAnalysisWnd::onSelectionChanged);
    connect(noiseEdit, &QLineEdit::editingFinished, this, &BatteryOcvAnalysisWnd::onNoiseChanged);
    connect(saveButton, &QPushButton::clicked, this, &BatteryOcvAnalysisWnd::onSaveImage);

    plotSettings = BATTERYPARAMSPLOT_SettingsDefault();
    socOffset = 0;
    socScale = 1;
    tableUpdating = false;

    resize(1250, 720);
}

double BatteryOcvAnalysisWnd::noiseGet()
{
    double noise = noiseEdit->text().toDouble();

    return (noise > 0) ? noise : 0;
}

QColor BatteryOcvAnalysisWnd::degreeColor(int index)
{
    static const QColor colors[] = {
        QColor(200, 60, 20), QColor(30, 110, 200), QColor(30, 150, 60),
        QColor(120, 60, 180), QColor(210, 150, 20), QColor(20, 150, 170)
    };

    return colors[index % 6];
}

/*OCV is fitted as a function of the state of charge, the same way the reference
  scripts do it. SoC is normalised first, otherwise the normal equations of a
  twentieth order polynomial are hopeless*/
bool BatteryOcvAnalysisWnd::polynomialFit(int degree, QVector<double> *coefficients)
{
    int size = degree + 1;
    QVector<QVector<double>> matrix(size, QVector<double>(size + 1, 0));

    coefficients->clear();

    if(measuredSoc.size() < size + 1) return false;

    for(int i = 0; i < measuredSoc.size(); i++)
    {
        QVector<double> basis(size);
        double normalized = (measuredSoc[i] - socOffset) / socScale;

        basis[0] = 1.0;
        for(int power = 1; power < size; power++) basis[power] = basis[power - 1] * normalized;

        for(int row = 0; row < size; row++)
        {
            for(int column = 0; column < size; column++)
            {
                matrix[row][column] += basis[row] * basis[column];
            }
            matrix[row][size] += basis[row] * measuredOcv[i];
        }
    }

    for(int column = 0; column < size; column++)
    {
        int pivot = column;

        for(int row = column + 1; row < size; row++)
        {
            if(fabs(matrix[row][column]) > fabs(matrix[pivot][column])) pivot = row;
        }

        if(fabs(matrix[pivot][column]) < 1e-18) return false;

        matrix.swapItemsAt(column, pivot);

        for(int row = 0; row < size; row++)
        {
            double factor;

            if(row == column) continue;

            factor = matrix[row][column] / matrix[column][column];

            for(int i = column; i < size + 1; i++)
            {
                matrix[row][i] -= factor * matrix[column][i];
            }
        }
    }

    for(int row = 0; row < size; row++)
    {
        coefficients->append(matrix[row][size] / matrix[row][row]);
    }

    return true;
}

double BatteryOcvAnalysisWnd::polynomialValue(const QVector<double> &coefficients, double soc)
{
    double normalized = (soc - socOffset) / socScale;
    double power = 1.0;
    double value = 0;

    for(int i = 0; i < coefficients.size(); i++)
    {
        value += coefficients[i] * power;
        power *= normalized;
    }

    return value;
}

/*dV/dSoC in volts per percent*/
double BatteryOcvAnalysisWnd::polynomialSlope(const QVector<double> &coefficients, double soc)
{
    double normalized = (soc - socOffset) / socScale;
    double power = 1.0;
    double value = 0;

    for(int i = 1; i < coefficients.size(); i++)
    {
        value += i * coefficients[i] * power;
        power *= normalized;
    }

    return value / socScale;
}

void BatteryOcvAnalysisWnd::setData(QVector<double> soc, QVector<double> ocv)
{
    QVector<int> order;

    measuredSoc.clear();
    measuredOcv.clear();

    for(int i = 0; i < soc.size() && i < ocv.size(); i++) order.append(i);

    std::sort(order.begin(), order.end(), [&](int a, int b){ return soc[a] < soc[b]; });

    for(int i = 0; i < order.size(); i++)
    {
        measuredSoc.append(soc[order[i]]);
        measuredOcv.append(ocv[order[i]]);
    }

    socOffset = 0;
    socScale = 0;

    for(int i = 0; i < measuredSoc.size(); i++) socOffset += measuredSoc[i];
    if(!measuredSoc.isEmpty()) socOffset /= measuredSoc.size();

    for(int i = 0; i < measuredSoc.size(); i++)
    {
        if(fabs(measuredSoc[i] - socOffset) > socScale) socScale = fabs(measuredSoc[i] - socOffset);
    }

    if(socScale <= 0) socScale = 1;

    buildFits();
    updateAll();
}

void BatteryOcvAnalysisWnd::buildFits()
{
    int minimumDegree = degreeMinEdit->text().toInt();
    int maximumDegree = degreeMaxEdit->text().toInt();
    int bestRow = -1;
    double bestRmse = -1;

    if(minimumDegree < 1) minimumDegree = 1;
    if(maximumDegree > BATTERYOCV_DEGREE_LIMIT) maximumDegree = BATTERYOCV_DEGREE_LIMIT;
    if(maximumDegree > measuredSoc.size() - 2) maximumDegree = measuredSoc.size() - 2;
    if(maximumDegree < minimumDegree) maximumDegree = minimumDegree;

    degreeMinEdit->setText(QString::number(minimumDegree));
    degreeMaxEdit->setText(QString::number(maximumDegree));

    fits.clear();
    tableUpdating = true;
    degreeTable->setRowCount(0);

    for(int degree = minimumDegree; degree <= maximumDegree; degree++)
    {
        batteryocv_fit_t fit;
        double squareSum = 0;
        double absoluteSum = 0;
        double errorSum = 0;
        int row;

        fit.degree = degree;
        fit.valid = polynomialFit(degree, &fit.coefficients);
        fit.rmse = 0;
        fit.meanError = 0;
        fit.standardDeviation = 0;

        if(!fit.valid) continue;

        for(int i = 0; i < measuredSoc.size(); i++)
        {
            double error = polynomialValue(fit.coefficients, measuredSoc[i]) - measuredOcv[i];

            squareSum += error * error;
            absoluteSum += fabs(error);
            errorSum += error;
        }

        fit.rmse = sqrt(squareSum / measuredSoc.size()) * 1000.0;
        fit.meanError = absoluteSum / measuredSoc.size() * 1000.0;
        fit.standardDeviation = sqrt(squareSum / measuredSoc.size() -
                                     (errorSum / measuredSoc.size()) * (errorSum / measuredSoc.size())) * 1000.0;

        fits.append(fit);

        row = degreeTable->rowCount();
        degreeTable->insertRow(row);

        QTableWidgetItem *drawItem = new QTableWidgetItem();
        drawItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        drawItem->setCheckState(Qt::Unchecked);
        degreeTable->setItem(row, 0, drawItem);
        degreeTable->setItem(row, 1, prvBATTERYOCV_ItemCreate(QString::number(degree)));
        degreeTable->setItem(row, 2, prvBATTERYOCV_ItemCreate(QString::number(fit.rmse, 'f', 3)));
        degreeTable->setItem(row, 3, prvBATTERYOCV_ItemCreate(QString::number(fit.meanError, 'f', 3)));
        degreeTable->setItem(row, 4, prvBATTERYOCV_ItemCreate(QString::number(fit.standardDeviation, 'f', 3)));

        if((bestRmse < 0) || (fit.rmse < bestRmse))
        {
            bestRmse = fit.rmse;
            bestRow = row;
        }
    }

    /*The degree with the smallest error starts ticked, everything else is left
      to the user*/
    if(bestRow >= 0)
    {
        degreeTable->item(bestRow, 0)->setCheckState(Qt::Checked);
    }

    tableUpdating = false;
}

void BatteryOcvAnalysisWnd::updateOcvPlot()
{
    int colorIndex = 0;

    ocvPlot->clearGraphs();

    if(measuredCheckBox->isChecked())
    {
        QCPGraph *measuredGraph = ocvPlot->addGraph();

        measuredGraph->setName("Measured");
        measuredGraph->setPen(QPen(QColor(60, 60, 60), 2));
        measuredGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(60, 60, 60),
                                                       QColor(60, 60, 60), 7));
        measuredGraph->setData(measuredSoc, measuredOcv, true);
    }

    for(int i = 0; i < fits.size() && i < degreeTable->rowCount(); i++)
    {
        QVector<double> curveSoc;
        QVector<double> curveOcv;
        QCPGraph *graph;

        if(degreeTable->item(i, 0)->checkState() != Qt::Checked) continue;

        for(int point = 0; point < BATTERYOCV_CURVE_POINTS; point++)
        {
            double soc = measuredSoc.first() + (measuredSoc.last() - measuredSoc.first()) *
                         (double)point / (BATTERYOCV_CURVE_POINTS - 1);

            curveSoc.append(soc);
            curveOcv.append(polynomialValue(fits[i].coefficients, soc));
        }

        graph = ocvPlot->addGraph();
        graph->setName("Degree " + QString::number(fits[i].degree));
        graph->setPen(QPen(degreeColor(colorIndex), 2, Qt::DashLine));
        graph->setData(curveSoc, curveOcv, true);
        colorIndex++;
    }

    ocvPlot->rescaleAxes();
    BATTERYPARAMSPLOT_Apply(ocvPlot, plotSettings);
}

void BatteryOcvAnalysisWnd::updateSensitivityPlot()
{
    int colorIndex = 0;
    QVector<double> magnitudes;

    sensitivityPlot->clearGraphs();

    /*Sensitivity of the measured points, taken between neighbouring cycles*/
    if(measuredCheckBox->isChecked() && (measuredSoc.size() > 1))
    {
        QVector<double> rawSoc;
        QVector<double> rawSensitivity;
        QCPGraph *graph;

        for(int i = 0; i < measuredSoc.size() - 1; i++)
        {
            double deltaVoltage = measuredOcv[i + 1] - measuredOcv[i];
            double deltaSoc = measuredSoc[i + 1] - measuredSoc[i];

            if(fabs(deltaVoltage) < 1e-9) continue;

            rawSoc.append((measuredSoc[i] + measuredSoc[i + 1]) / 2.0);
            rawSensitivity.append(deltaSoc / deltaVoltage);
            magnitudes.append(fabs(deltaSoc / deltaVoltage));
        }

        graph = sensitivityPlot->addGraph();
        graph->setName("Measured");
        graph->setPen(QPen(QColor(60, 60, 60), 2));
        graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(60, 60, 60),
                                               QColor(60, 60, 60), 6));
        graph->setData(rawSoc, rawSensitivity, true);
    }

    for(int i = 0; i < fits.size() && i < degreeTable->rowCount(); i++)
    {
        QVector<double> curveSoc;
        QVector<double> curveSensitivity;
        QCPGraph *graph;

        if(degreeTable->item(i, 0)->checkState() != Qt::Checked) continue;

        for(int point = 0; point < BATTERYOCV_CURVE_POINTS; point++)
        {
            double soc = measuredSoc.first() + (measuredSoc.last() - measuredSoc.first()) *
                         (double)point / (BATTERYOCV_CURVE_POINTS - 1);
            double slope = polynomialSlope(fits[i].coefficients, soc);

            if(fabs(slope) < 1e-9) continue;

            curveSoc.append(soc);
            curveSensitivity.append(1.0 / slope);
            magnitudes.append(fabs(1.0 / slope));
        }

        graph = sensitivityPlot->addGraph();
        graph->setName("Degree " + QString::number(fits[i].degree));
        graph->setPen(QPen(degreeColor(colorIndex), 2, Qt::DashLine));
        graph->setData(curveSoc, curveSensitivity, true);
        colorIndex++;
    }

    sensitivityPlot->rescaleAxes();

    /*Where the fitted curve turns, its slope passes through zero and the
      sensitivity goes to infinity. Those spikes would flatten everything else,
      so the visible range follows the bulk of the values and the spikes are left
      to be reached by zooming out*/
    if(!magnitudes.isEmpty())
    {
        double limit;

        std::sort(magnitudes.begin(), magnitudes.end());
        limit = magnitudes[(int)(magnitudes.size() * BATTERYOCV_SENSITIVITY_PERCENTILE)] * BATTERYOCV_SENSITIVITY_MARGIN;

        if(limit > 0) sensitivityPlot->yAxis->setRange(-limit, limit);
    }

    BATTERYPARAMSPLOT_Apply(sensitivityPlot, plotSettings);
}

void BatteryOcvAnalysisWnd::updateErrorPlot()
{
    QVector<double> degrees;
    QVector<double> rmse;
    QVector<double> meanError;
    QVector<double> standardDeviation;
    QCPGraph *graph;

    errorPlot->clearGraphs();

    for(int i = 0; i < fits.size(); i++)
    {
        degrees.append(fits[i].degree);
        rmse.append(fits[i].rmse);
        meanError.append(fits[i].meanError);
        standardDeviation.append(fits[i].standardDeviation);
    }

    graph = errorPlot->addGraph();
    graph->setName("RMSE");
    graph->setPen(QPen(QColor(200, 60, 20), 2));
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(200, 60, 20), QColor(200, 60, 20), 6));
    graph->setData(degrees, rmse, true);

    graph = errorPlot->addGraph();
    graph->setName("Mean absolute error");
    graph->setPen(QPen(QColor(30, 110, 200), 2));
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(30, 110, 200), QColor(30, 110, 200), 6));
    graph->setData(degrees, meanError, true);

    graph = errorPlot->addGraph();
    graph->setName("Standard deviation");
    graph->setPen(QPen(QColor(30, 150, 60), 2));
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(30, 150, 60), QColor(30, 150, 60), 6));
    graph->setData(degrees, standardDeviation, true);

    errorPlot->rescaleAxes();
    BATTERYPARAMSPLOT_Apply(errorPlot, plotSettings);
}

void BatteryOcvAnalysisWnd::updateSummary()
{
    double noise = noiseGet();
    int selected = -1;

    for(int i = 0; i < fits.size() && i < degreeTable->rowCount(); i++)
    {
        if(degreeTable->item(i, 0)->checkState() != Qt::Checked) continue;
        selected = i;
        break;
    }

    if(selected < 0)
    {
        summaryLabel->setText("Tick a polynomial degree to see how the voltage noise translates into a state of charge error.");
        return;
    }

    double sensitivityMean = 0;
    double sensitivityMax = 0;
    double sensitivityMaxSoc = 0;
    QVector<double> magnitudes;
    int pointNo = 0;

    for(int point = 0; point < BATTERYOCV_CURVE_POINTS; point++)
    {
        double soc = measuredSoc.first() + (measuredSoc.last() - measuredSoc.first()) *
                     (double)point / (BATTERYOCV_CURVE_POINTS - 1);
        double slope = polynomialSlope(fits[selected].coefficients, soc);
        double sensitivity;

        if(fabs(slope) < 1e-9) continue;

        sensitivity = fabs(1.0 / slope);

        magnitudes.append(sensitivity);
        sensitivityMean += sensitivity;
        pointNo++;

        if(sensitivity <= sensitivityMax) continue;

        sensitivityMax = sensitivity;
        sensitivityMaxSoc = soc;
    }

    if(pointNo == 0)
    {
        summaryLabel->setText("Sensitivity could not be evaluated for the selected degree.");
        return;
    }

    /*Average is taken over the bulk of the curve for the same reason the plot
      limits its range: a single turning point would otherwise dominate it*/
    std::sort(magnitudes.begin(), magnitudes.end());
    sensitivityMean = magnitudes[magnitudes.size() / 2];

    summaryLabel->setText(QString("Degree %1 used for the numbers below.\n\n"
                                  "Sensitivity dSoC/dV:\n"
                                  "  median %2 %/V\n"
                                  "  worst %3 %/V at %4 % SoC\n\n"
                                  "With %5 mV of voltage noise:\n"
                                  "  typical SoC error %6 %\n"
                                  "  worst SoC error %7 %\n\n"
                                  "The flatter the OCV curve, the larger the SoC error the same voltage "
                                  "noise produces.")
                          .arg(fits[selected].degree)
                          .arg(sensitivityMean, 0, 'f', 2)
                          .arg(sensitivityMax, 0, 'f', 2)
                          .arg(sensitivityMaxSoc, 0, 'f', 1)
                          .arg(noise)
                          .arg(sensitivityMean * noise / 1000.0, 0, 'f', 2)
                          .arg(sensitivityMax * noise / 1000.0, 0, 'f', 2));
}

void BatteryOcvAnalysisWnd::setPlotSettings(batteryparams_plot_settings_t settings)
{
    plotSettings = settings;

    BATTERYPARAMSPLOT_Apply(ocvPlot, plotSettings);
    BATTERYPARAMSPLOT_Apply(sensitivityPlot, plotSettings);
    BATTERYPARAMSPLOT_Apply(errorPlot, plotSettings);
}

void BatteryOcvAnalysisWnd::updateAll()
{
    updateOcvPlot();
    updateSensitivityPlot();
    updateErrorPlot();
    updateSummary();
}

void BatteryOcvAnalysisWnd::onDegreeRangeChanged()
{
    buildFits();
    updateAll();
}

void BatteryOcvAnalysisWnd::onSelectionChanged()
{
    if(tableUpdating) return;

    updateOcvPlot();
    updateSensitivityPlot();
    updateSummary();
}

void BatteryOcvAnalysisWnd::onNoiseChanged()
{
    updateSummary();
}

void BatteryOcvAnalysisWnd::onSaveImage()
{
    QCustomPlot *plot = qobject_cast<QCustomPlot*>(tabWidget->currentWidget());

    if(plot == NULL) return;

    BATTERYPARAMSPLOT_Save(plot, this,
                           tabWidget->tabText(tabWidget->currentIndex()).toLower().replace(' ', '_').replace('/', '_'));
}
