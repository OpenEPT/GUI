#include "autocalibrationwnd.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QFile>
#include <math.h>
#include <QTime>
#include <QInputDialog>

#define AUTOCAL_VOLTAGE_REFERENCE       2.049
#define AUTOCAL_FLOAT_VOLTAGE_MIN       5.000
#define AUTOCAL_ZERO_VOLTAGE_LIMIT      0.100
#define AUTOCAL_CONNECTED_VOLTAGE_MIN   0.300
#define AUTOCAL_CONNECTED_VOLTAGE_MAX   4.600
#define AUTOCAL_VOLTAGE_SPREAD_LIMIT    0.010
#define AUTOCAL_CURRENT_SPREAD_LIMIT    2.000
#define AUTOCAL_VOLTAGE_TOLERANCE       0.005
#define AUTOCAL_CURRENT_TOLERANCE       0.500
#define AUTOCAL_HISTORY_DEPTH           6
#define AUTOCAL_LOAD_SETTLE_TICKS       8
#define AUTOCAL_LOAD_MIN_CURRENT        100.0
#define AUTOCAL_LOAD_POINTS_MIN         3
#define AUTOCAL_LOAD_POINTS_MAX         8
#define AUTOCAL_CURRENT_SPAN_TARGET     200.0

AutoCalibrationWnd::AutoCalibrationWnd(QWidget *parent) :
    QWidget(parent)
{
    QFont defaultFont("Arial", 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    setFont(defaultFont);
    setWindowTitle("Automatic calibration");
    resize(520, 520);

    calData = NULL;
    step = AUTOCAL_STEP_IDLE;
    acquisitionActive = false;
    loadDisabled = true;
    referenceVoltage = AUTOCAL_VOLTAGE_REFERENCE;
    externalReference = false;
    voltageAvg = 0;
    currentAvg = 0;
    statsValid = false;
    loadPhase = 0;
    loadSettleTicks = 0;
    snapshotValid = false;

    titleLabel = new QLabel(this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    stepLabel = new QLabel(this);
    stepLabel->setStyleSheet("color: gray;");
    instructionLabel = new QLabel(this);
    instructionLabel->setWordWrap(true);
    instructionLabel->setMinimumHeight(70);
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumHeight(160);
    liveLabel = new QLabel(this);
    liveLabel->setStyleSheet("font-size: 13px;");
    resultLabel = new QLabel(this);
    resultLabel->setWordWrap(true);
    resultLabel->setStyleSheet("font-weight: bold;");

    logView = new QPlainTextEdit(this);
    logView->setReadOnly(true);
    logView->setMaximumBlockCount(500);
    logView->setMinimumHeight(110);
    logView->setStyleSheet("font-family: monospace; font-size: 11px;");

    backButton = new QPushButton("Back", this);
    cancelButton = new QPushButton("Cancel", this);
    readyButton = new QPushButton("I am ready, continue", this);
    readyButton->setToolTip("Continue to the next step without waiting for the automatic detection");
    nextButton = new QPushButton("Next", this);

    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(backButton);
    buttonLayout->addWidget(readyButton);
    buttonLayout->addWidget(nextButton);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(stepLabel);
    mainLayout->addWidget(instructionLabel);
    mainLayout->addWidget(imageLabel, 1);
    mainLayout->addWidget(liveLabel);
    mainLayout->addWidget(resultLabel);
    mainLayout->addWidget(logView, 1);
    mainLayout->addLayout(buttonLayout);

    loadTimer = new QTimer(this);
    loadTimer->setInterval(500);

    connect(nextButton, &QPushButton::clicked, this, &AutoCalibrationWnd::onNextClicked);
    connect(backButton, &QPushButton::clicked, this, &AutoCalibrationWnd::onBackClicked);
    connect(cancelButton, &QPushButton::clicked, this, &AutoCalibrationWnd::onCancelClicked);
    connect(readyButton, &QPushButton::clicked, this, &AutoCalibrationWnd::onReadyClicked);
    connect(loadTimer, &QTimer::timeout, this, &AutoCalibrationWnd::onLoadStepTick);
}

void AutoCalibrationWnd::setCalibrationData(CalibrationData *aCalData)
{
    calData = aCalData;
}

void AutoCalibrationWnd::setAcquisitionActive(bool active)
{
    acquisitionActive = active;
}

void AutoCalibrationWnd::setLoadDisabled(bool disabled)
{
    loadDisabled = disabled;
}

double AutoCalibrationWnd::maxLoadCurrentMa()
{
    if(calData == NULL || calData->currentShunt <= 0) return 1000.0;

    /*Shunt 0.045 ohm corresponds to a 3 A maximum, so the full scale current scales
      inversely with the shunt value*/
    return 135.0 / calData->currentShunt;
}

int AutoCalibrationWnd::loadPointCount()
{
    /*At least three points, more for a wider current range so the line is well
      constrained*/
    int points = (int)(maxLoadCurrentMa() / 500.0) + AUTOCAL_LOAD_POINTS_MIN;

    if(points < AUTOCAL_LOAD_POINTS_MIN) points = AUTOCAL_LOAD_POINTS_MIN;
    if(points > AUTOCAL_LOAD_POINTS_MAX) points = AUTOCAL_LOAD_POINTS_MAX;

    return points;
}

void AutoCalibrationWnd::setImage(QString fileName)
{
    QString path = ":/images/Calibration/" + fileName;

    if(QFile::exists(path))
    {
        imageLabel->setPixmap(QPixmap(path).scaledToHeight(150, Qt::SmoothTransformation));
    }
    else
    {
        imageLabel->setPixmap(QPixmap());
    }
}

void AutoCalibrationWnd::pushHistory(double v, double c)
{
    voltageHistory.append(v);
    currentHistory.append(c);

    while(voltageHistory.size() > AUTOCAL_HISTORY_DEPTH) voltageHistory.removeFirst();
    while(currentHistory.size() > AUTOCAL_HISTORY_DEPTH) currentHistory.removeFirst();
}

bool AutoCalibrationWnd::voltageStable(double *mean, double spreadLimit)
{
    double minimum;
    double maximum;
    double sum = 0;

    if(voltageHistory.size() < AUTOCAL_HISTORY_DEPTH) return false;

    minimum = voltageHistory.first();
    maximum = voltageHistory.first();

    for(int i = 0; i < voltageHistory.size(); i++)
    {
        if(voltageHistory[i] < minimum) minimum = voltageHistory[i];
        if(voltageHistory[i] > maximum) maximum = voltageHistory[i];
        sum += voltageHistory[i];
    }

    if(mean != NULL) *mean = sum / voltageHistory.size();

    return (maximum - minimum) <= spreadLimit;
}

bool AutoCalibrationWnd::currentStable(double *mean, double spreadLimit)
{
    double minimum;
    double maximum;
    double sum = 0;

    if(currentHistory.size() < AUTOCAL_HISTORY_DEPTH) return false;

    minimum = currentHistory.first();
    maximum = currentHistory.first();

    for(int i = 0; i < currentHistory.size(); i++)
    {
        if(currentHistory[i] < minimum) minimum = currentHistory[i];
        if(currentHistory[i] > maximum) maximum = currentHistory[i];
        sum += currentHistory[i];
    }

    if(mean != NULL) *mean = sum / currentHistory.size();

    return (maximum - minimum) <= spreadLimit;
}

void AutoCalibrationWnd::startCalibration()
{
    if(!acquisitionActive)
    {
        QMessageBox::warning(this, "Automatic calibration",
                             "Start the acquisition before the calibration.\n\n"
                             "The calibration uses the real time averaged voltage and current.");
        return;
    }

    QMessageBox box(this);
    box.setWindowTitle("Automatic calibration");
    box.setText("Which voltage reference is used for the voltage channel?");
    QPushButton *internalBtn = box.addButton("Internal 2.049 V", QMessageBox::AcceptRole);
    QPushButton *externalBtn = box.addButton("External value", QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();

    if(box.clickedButton() == externalBtn)
    {
        bool ok = false;
        double value = QInputDialog::getDouble(this, "External reference",
                                               "External reference voltage held on the input [V]:",
                                               2.049, 0.05, 20.0, 4, &ok);
        if(!ok) return;

        externalReference = true;
        referenceVoltage = value;
    }
    else if(box.clickedButton() == internalBtn)
    {
        externalReference = false;
        referenceVoltage = AUTOCAL_VOLTAGE_REFERENCE;
    }
    else
    {
        return;
    }

    {
        bool ok = false;
        int defaultPoints = loadPointCount();

        currentSpanPoints = QInputDialog::getInt(this, "Current span points",
                            "Number of current points, you enter the reference current at each (min 3):",
                            qMax(3, defaultPoints), 3, 20, 1, &ok);
        if(!ok) return;

        loadPoints = QInputDialog::getInt(this, "Load calibration points",
                     "Number of load points (no input needed, measured automatically):",
                     defaultPoints, 2, 20, 1, &ok);
        if(!ok) return;
    }

    voltageHistory.clear();
    currentHistory.clear();
    logView->clear();
    appendLog("Calibration started");
    appendLog("Current span points: " + QString::number(currentSpanPoints)
              + ", load points: " + QString::number(loadPoints));
    appendLog(externalReference ? ("External reference " + QString::number(referenceVoltage, 'f', 4)
                                   + " V, jumper steps skipped")
                                : "Internal reference 2.049 V");

    if(calData != NULL)
    {
        startSnapshot = *calData;
        snapshotValid = true;
    }
    goToStep(externalReference ? AUTOCAL_STEP_VOLTAGE_TUNE : AUTOCAL_STEP_DISCONNECT_INPUT);
    show();
    raise();
    activateWindow();
}

void AutoCalibrationWnd::appendLog(QString message)
{
    logView->appendPlainText(QTime::currentTime().toString("HH:mm:ss") + "  " + message);
}

QString AutoCalibrationWnd::buildReport()
{
    QString report;
    int changed = 0;

    if(!snapshotValid || calData == NULL) return "No calibration data.";

    struct { const char *name; double before; double after; int decimals; } rows[] = {
        { "ADC voltage reference", startSnapshot.adcVoltageRef,     calData->adcVoltageRef,     4 },
        { "Voltage offset",        startSnapshot.voltageOff,        calData->voltageOff,        5 },
        { "Voltage correction",    startSnapshot.voltageCorr,       calData->voltageCorr,       5 },
        { "Voltage C. offset",     startSnapshot.voltageCurrOffset, calData->voltageCurrOffset, 5 },
        { "Current correction",    startSnapshot.currentCorrection, calData->currentCorrection, 5 },
        { "Current gain",          startSnapshot.currentGain,       calData->currentGain,       4 },
        { "Current shunt",         startSnapshot.currentShunt,      calData->currentShunt,      4 },
        { "Load DAC offset",       startSnapshot.dacOffset,         calData->dacOffset,         2 },
        { "Load DAC correction",   startSnapshot.dacCorrection,     calData->dacCorrection,     4 }
    };

    report = "Changed calibration coefficients:\n";

    for(unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); i++)
    {
        if(fabs(rows[i].after - rows[i].before) < 1e-9) continue;

        report += QString("  %1:  %2  ->  %3\n")
                  .arg(rows[i].name)
                  .arg(rows[i].before, 0, 'f', rows[i].decimals)
                  .arg(rows[i].after, 0, 'f', rows[i].decimals);
        changed++;
    }

    if(changed == 0) report += "  none\n";

    return report;
}

autocal_step_t AutoCalibrationWnd::modeStartStep(autocal_step_t s)
{
    /*Keep the already calibrated channels, restart only the channel that failed*/
    if(s <= AUTOCAL_STEP_VOLTAGE_TUNE) return externalReference ? AUTOCAL_STEP_VOLTAGE_TUNE : AUTOCAL_STEP_DISCONNECT_INPUT;
    if(s == AUTOCAL_STEP_CURRENT_OFFSET) return AUTOCAL_STEP_CURRENT_OFFSET;
    if(s == AUTOCAL_STEP_CURRENT_SPAN) return AUTOCAL_STEP_CURRENT_SPAN;
    if(s == AUTOCAL_STEP_CURRENT_ZERO2) return AUTOCAL_STEP_CURRENT_ZERO2;
    return externalReference ? AUTOCAL_STEP_CONNECT_SOURCE : AUTOCAL_STEP_JUMPER_BATTERY;
}

void AutoCalibrationWnd::goToStep(autocal_step_t newStep)
{
    step = newStep;
    voltageHistory.clear();
    currentHistory.clear();
    loadTimer->stop();
    resultLabel->clear();
    updateView();
    appendLog("Step: " + stepLabel->text());

    if(step == AUTOCAL_STEP_CURRENT_SPAN)
    {
        int points = currentSpanPoints;
        double maxMa = maxLoadCurrentMa() * 0.8;
        double first = AUTOCAL_LOAD_MIN_CURRENT;

        if(points < 2) points = 2;
        if(maxMa < first + 1.0) maxMa = first + 1.0;

        loadRequested.clear();
        for(int i = 0; i < points; i++)
        {
            loadRequested.append(first + (maxMa - first) * i / (double)(points - 1));
        }

        loadPhase = 0;
        loadSettleTicks = 0;
        spanSumMT = 0;
        spanSumMM = 0;

        appendLog("Current span: " + QString::number(points) + " points from "
                  + QString::number((int)first) + " mA to " + QString::number((int)maxMa)
                  + " mA, you enter the reference at each");

        loadDisabled = false;
        emit sigResetProtection();
        emit sigSetLoadCurrent((int)loadRequested.first());
        emit sigSetLoadEnabled(true);
        loadTimer->start();
    }

    if(step == AUTOCAL_STEP_LOAD_TUNE)
    {
        {
            int points = loadPoints;
            double maxMa = maxLoadCurrentMa() * 0.8;
            double first = AUTOCAL_LOAD_MIN_CURRENT;

            if(maxMa < first + 1.0) maxMa = first + 1.0;

            loadRequested.clear();
            loadMeasured.clear();

            for(int i = 0; i < points; i++)
            {
                loadRequested.append(first + (maxMa - first) * i / (double)(points - 1));
            }

            loadPhase = 0;
            loadSettleTicks = 0;

            appendLog("Load tuning: " + QString::number(points) + " points from "
                      + QString::number((int)first) + " mA to " + QString::number((int)maxMa)
                      + " mA (max " + QString::number((int)maxLoadCurrentMa()) + " mA)");

            loadDisabled = false;
            emit sigResetProtection();
            emit sigSetLoadCurrent((int)loadRequested.first());
            emit sigSetLoadEnabled(true);
            loadTimer->start();
        }
    }
}

void AutoCalibrationWnd::updateView()
{
    backButton->setEnabled(step > AUTOCAL_STEP_DISCONNECT_INPUT && step < AUTOCAL_STEP_DONE);
    nextButton->setText(step == AUTOCAL_STEP_DONE ? "Finish" : "Next");
    nextButton->setEnabled(step == AUTOCAL_STEP_DONE);
    readyButton->setEnabled(step == AUTOCAL_STEP_DISCONNECT_INPUT ||
                            step == AUTOCAL_STEP_SET_REFERENCE ||
                            step == AUTOCAL_STEP_JUMPER_BATTERY ||
                            step == AUTOCAL_STEP_CONNECT_SOURCE);

    switch(step)
    {
    case AUTOCAL_STEP_DISCONNECT_INPUT:
        titleLabel->setText("Voltage channel calibration");
        stepLabel->setText("Step 1 of 3  -  disconnect the input");
        instructionLabel->setText("Disconnect everything from the Power Input connector.\n"
                                  "The calibration continues on its own once the input reading drops to zero "
                                  "or floats above " + QString::number(AUTOCAL_FLOAT_VOLTAGE_MIN, 'f', 1) +
                                  " V, or press \"I am ready, continue\".");
        setImage("input_disconnect.png");
        break;
    case AUTOCAL_STEP_SET_REFERENCE:
        titleLabel->setText("Voltage channel calibration");
        stepLabel->setText("Step 1 of 3  -  select the voltage reference");
        instructionLabel->setText("Move the ADC IN Selection jumper to the voltage reference position.\n"
                                  "The voltage channel is tuned automatically once the reference is detected, "
                                  "or press \"I am ready, continue\".");
        setImage("adc_in_selection.png");
        break;
    case AUTOCAL_STEP_VOLTAGE_TUNE:
        titleLabel->setText("Voltage channel calibration");
        stepLabel->setText("Step 1 of 3  -  tuning");
        instructionLabel->setText("Tuning the voltage correction so the measured voltage reaches "
                                  + QString::number(referenceVoltage, 'f', 3) + " V."
                                  + QString(externalReference ? "\nKeep the external reference connected to the input." : ""));
        break;
    case AUTOCAL_STEP_CURRENT_OFFSET:
        titleLabel->setText("Current channel calibration");
        stepLabel->setText("Step 2 of 3  -  zero current");
        instructionLabel->setText("Make sure the Load is disabled.\n"
                                  "The current offset is tuned automatically so the measured current gets as "
                                  "close to zero as possible (the measurement is bidirectional).");
        setImage("");
        break;
    case AUTOCAL_STEP_CURRENT_ZERO2:
        titleLabel->setText("Current channel calibration");
        stepLabel->setText("Step 2 of 3  -  zero re-check");
        instructionLabel->setText("Make sure the Load is disabled.\n"
                                  "The current offset is checked again after the span before the load calibration.");
        setImage("");
        break;
    case AUTOCAL_STEP_JUMPER_BATTERY:
        titleLabel->setText("Load calibration");
        stepLabel->setText("Step 3 of 3  -  select the battery");
        instructionLabel->setText("Move the ADC IN Selection jumper back to the Battery position.\n"
                                  "Wait until the input reading drops to zero or floats above "
                                  + QString::number(AUTOCAL_FLOAT_VOLTAGE_MIN, 'f', 1) +
                                  " V, or press \"I am ready, continue\".");
        setImage("adc_in_selection.png");
        break;
    case AUTOCAL_STEP_CONNECT_SOURCE:
        titleLabel->setText("Load calibration");
        stepLabel->setText("Step 3 of 3  -  connect the source");
        instructionLabel->setText("Connect a battery or a generator to the Power Input connector.\n"
                                  "The calibration continues once the input voltage rises, "
                                  "or press \"I am ready, continue\".");
        setImage("input_disconnect.png");
        break;
    case AUTOCAL_STEP_CURRENT_SPAN:
        titleLabel->setText("Current channel calibration");
        stepLabel->setText("Step 2 of 3  -  span");
        instructionLabel->setText("Driving a known current through the load.\n"
                                  "Read the true current on your reference meter, the wizard will ask for it.");
        setImage("");
        break;
    case AUTOCAL_STEP_LOAD_TUNE:
        titleLabel->setText("Load calibration");
        stepLabel->setText("Step 3 of 3  -  tuning");
        instructionLabel->setText("Driving two load currents and tuning the load DAC so the requested "
                                  "current matches the measured current.\nKeep the source connected.");
        setImage("");
        break;
    case AUTOCAL_STEP_DONE:
    {
        QString report = buildReport();

        titleLabel->setText("Automatic calibration finished");
        stepLabel->setText("All channels calibrated");
        instructionLabel->setText(report +
                                  "\nThe parameters are kept in the application. Open the Calibration "
                                  "window and press Store to keep them on the device.");
        liveLabel->clear();
        setImage("");
        appendLog(report);
        break;
    }
    default:
        break;
    }
}

void AutoCalibrationWnd::failAndRestart(QString reason)
{
    autocal_step_t restart = modeStartStep(step);

    resultLabel->setStyleSheet("font-weight: bold; color: rgb(170, 0, 0);");
    resultLabel->setText(reason);
    appendLog("FAILED: " + reason + " - restarting this channel");
    QMessageBox::warning(this, "Automatic calibration",
                         reason + "\n\nThe calibration restarts this channel, the already calibrated channels are kept.");
    goToStep(restart);
}

void AutoCalibrationWnd::onNewStatistics(double aVoltageAvg, double aCurrentAvg)
{
    double mean;

    voltageAvg = aVoltageAvg;
    currentAvg = aCurrentAvg;
    statsValid = true;

    if(!isVisible() || calData == NULL) return;

    pushHistory(voltageAvg, currentAvg);

    liveLabel->setText("Measured  -  voltage " + QString::number(voltageAvg, 'f', 4) + " V   |   current "
                       + QString::number(currentAvg, 'f', 2) + " mA");

    switch(step)
    {
    case AUTOCAL_STEP_DISCONNECT_INPUT:
        /*Depending on the wiring a free input either floats high above the range or
          sits near zero, so both count as disconnected*/
        if(voltageStable(&mean, AUTOCAL_VOLTAGE_SPREAD_LIMIT) &&
           ((mean > AUTOCAL_FLOAT_VOLTAGE_MIN) || (fabs(mean) < AUTOCAL_ZERO_VOLTAGE_LIMIT)))
        {
            appendLog("Input free detected at " + QString::number(mean, 'f', 3) + " V");
            goToStep(AUTOCAL_STEP_SET_REFERENCE);
        }
        break;

    case AUTOCAL_STEP_SET_REFERENCE:
        if(voltageStable(&mean, AUTOCAL_VOLTAGE_SPREAD_LIMIT) &&
           (mean > AUTOCAL_CONNECTED_VOLTAGE_MIN) && (mean < qMax(AUTOCAL_CONNECTED_VOLTAGE_MAX, referenceVoltage * 1.5)))
        {
            appendLog("Reference detected at " + QString::number(mean, 'f', 3) + " V");
            goToStep(AUTOCAL_STEP_VOLTAGE_TUNE);
        }
        break;

    case AUTOCAL_STEP_VOLTAGE_TUNE:
        if(voltageStable(&mean, AUTOCAL_VOLTAGE_SPREAD_LIMIT))
        {
            if((mean > qMax(AUTOCAL_CONNECTED_VOLTAGE_MAX, referenceVoltage * 1.5)) || (mean < AUTOCAL_CONNECTED_VOLTAGE_MIN))
            {
                failAndRestart("Voltage reference lost during tuning.");
                break;
            }

            if(fabs(mean - referenceVoltage) <= AUTOCAL_VOLTAGE_TOLERANCE)
            {
                resultLabel->setStyleSheet("font-weight: bold; color: rgb(0, 130, 0);");
                resultLabel->setText("Voltage channel calibrated, correction "
                                     + QString::number(calData->voltageCorr, 'f', 5));
                appendLog("Voltage channel OK, measured " + QString::number(mean, 'f', 4) + " V");
                goToStep(AUTOCAL_STEP_CURRENT_OFFSET);
                break;
            }

            /*Measured voltage scales linearly with the correction, so the correction
              that reaches the reference is found in one step*/
            appendLog("Voltage " + QString::number(mean, 'f', 4) + " V, correction -> "
                      + QString::number(calData->voltageCorr * referenceVoltage / mean, 'f', 5));
            calData->voltageCorr = calData->voltageCorr * referenceVoltage / mean;
            emit sigApplyCalibration();
            voltageHistory.clear();
        }
        break;

    case AUTOCAL_STEP_CURRENT_OFFSET:
        if(!loadDisabled)
        {
            resultLabel->setStyleSheet("font-weight: bold; color: rgb(170, 0, 0);");
            resultLabel->setText("Disable the Load to calibrate the current offset.");
            break;
        }

        if(currentStable(&mean, AUTOCAL_CURRENT_SPREAD_LIMIT))
        {
            if(fabs(mean) <= AUTOCAL_CURRENT_TOLERANCE)
            {
                resultLabel->setStyleSheet("font-weight: bold; color: rgb(0, 130, 0);");
                resultLabel->setText("Current channel calibrated, offset "
                                     + QString::number(calData->voltageCurrOffset, 'f', 5));
                appendLog("Current zero OK, measured " + QString::number(mean, 'f', 2) + " mA");
                goToStep(externalReference ? AUTOCAL_STEP_CONNECT_SOURCE : AUTOCAL_STEP_JUMPER_BATTERY);
                break;
            }

            /*Current reads zero when the offset cancels the raw average, so the offset
              is shifted by the measured current expressed back in volts*/
            if((calData->currentShunt > 0) && (calData->currentGain > 0) && (fabs(calData->currentCorrection) > 1e-9))
            {
                appendLog("Current " + QString::number(mean, 'f', 2) + " mA, offset -> adjusting");
                calData->voltageCurrOffset += mean * (calData->currentShunt * calData->currentGain)
                                              / (1000.0 * calData->currentCorrection);
                emit sigApplyCalibration();
                currentHistory.clear();
            }
        }
        break;

    case AUTOCAL_STEP_CURRENT_ZERO2:
        if(!loadDisabled)
        {
            resultLabel->setStyleSheet("font-weight: bold; color: rgb(170, 0, 0);");
            resultLabel->setText("Disable the Load to calibrate the current offset.");
            break;
        }

        if(currentStable(&mean, AUTOCAL_CURRENT_SPREAD_LIMIT))
        {
            if(fabs(mean) <= AUTOCAL_CURRENT_TOLERANCE)
            {
                resultLabel->setStyleSheet("font-weight: bold; color: rgb(0, 130, 0);");
                resultLabel->setText("Current channel calibrated, offset "
                                     + QString::number(calData->voltageCurrOffset, 'f', 5));
                appendLog("Current zero re-check OK, measured " + QString::number(mean, 'f', 2) + " mA");
                goToStep(AUTOCAL_STEP_LOAD_TUNE);
                break;
            }

            /*Current reads zero when the offset cancels the raw average, so the offset
              is shifted by the measured current expressed back in volts*/
            if((calData->currentShunt > 0) && (calData->currentGain > 0) && (fabs(calData->currentCorrection) > 1e-9))
            {
                appendLog("Current " + QString::number(mean, 'f', 2) + " mA, offset -> adjusting");
                calData->voltageCurrOffset += mean * (calData->currentShunt * calData->currentGain)
                                              / (1000.0 * calData->currentCorrection);
                emit sigApplyCalibration();
                currentHistory.clear();
            }
        }
        break;

    case AUTOCAL_STEP_JUMPER_BATTERY:
        /*Input is still free, so switching the jumper back to the battery path makes
          the reading leave the connected band again, floating high or dropping to zero*/
        if(voltageStable(&mean, AUTOCAL_VOLTAGE_SPREAD_LIMIT) &&
           ((mean > AUTOCAL_FLOAT_VOLTAGE_MIN) || (fabs(mean) < AUTOCAL_ZERO_VOLTAGE_LIMIT)))
        {
            appendLog("Battery path selected, input free at " + QString::number(mean, 'f', 3) + " V");
            goToStep(AUTOCAL_STEP_CONNECT_SOURCE);
        }
        break;

    case AUTOCAL_STEP_CONNECT_SOURCE:
        if(voltageStable(&mean, AUTOCAL_VOLTAGE_SPREAD_LIMIT) &&
           (mean > AUTOCAL_CONNECTED_VOLTAGE_MIN) && (mean < AUTOCAL_CONNECTED_VOLTAGE_MAX))
        {
            appendLog("Source detected at " + QString::number(mean, 'f', 3) + " V");
            goToStep(AUTOCAL_STEP_CURRENT_SPAN);
        }
        break;

    default:
        break;
    }
}

void AutoCalibrationWnd::onLoadEnableResult(bool ok)
{
    if(step != AUTOCAL_STEP_LOAD_TUNE) return;
    if(ok) return;

    loadTimer->stop();
    failAndRestart("Unable to enable the load. Check that the protections are reset and the Load is allowed.");
}

void AutoCalibrationWnd::onLoadStepTick()
{
    double mean;

    if((step != AUTOCAL_STEP_LOAD_TUNE) && (step != AUTOCAL_STEP_CURRENT_SPAN)) { loadTimer->stop(); return; }

    loadSettleTicks++;

    if(loadSettleTicks < AUTOCAL_LOAD_SETTLE_TICKS) return;
    if(!currentStable(&mean, AUTOCAL_CURRENT_SPREAD_LIMIT * 4.0)) return;

    if(step == AUTOCAL_STEP_CURRENT_SPAN)
    {
        loadTimer->stop();

        bool ok = false;
        double trueCurrent = QInputDialog::getDouble(this, "Current span calibration",
                             "Point " + QString::number(loadPhase + 1) + " of " + QString::number(loadRequested.size())
                             + ".\nThe wizard measures " + QString::number(mean, 'f', 1) + " mA.\n"
                             "Enter the true current read on your reference meter [mA]:",
                             mean, 0.0, 100000.0, 2, &ok);

        if(!ok)
        {
            emit sigSetLoadEnabled(false);
            emit sigSetLoadCurrent(0);
            failAndRestart("Current span cancelled.");
            return;
        }

        spanSumMT += mean * trueCurrent;
        spanSumMM += mean * mean;
        appendLog("Span point " + QString::number(loadPhase + 1) + ": measured "
                  + QString::number(mean, 'f', 2) + " mA, reference " + QString::number(trueCurrent, 'f', 2) + " mA");

        loadPhase++;
        loadSettleTicks = 0;
        currentHistory.clear();

        if(loadPhase < loadRequested.size())
        {
            emit sigSetLoadCurrent((int)loadRequested[loadPhase]);
            loadTimer->start();
            return;
        }

        emit sigSetLoadEnabled(false);
        emit sigSetLoadCurrent(0);

        if(spanSumMM < 1e-9)
        {
            failAndRestart("No current measured for the span calibration.");
            return;
        }

        /*Current measurement is a pure scale once the offset is zeroed, so the scale
          that maps every measured value onto the reference reading is the least
          squares slope through the origin*/
        double scale = spanSumMT / spanSumMM;

        appendLog("Current correction -> " + QString::number(calData->currentCorrection * scale, 'f', 5));
        calData->currentCorrection = calData->currentCorrection * scale;
        emit sigApplyCalibration();

        resultLabel->setStyleSheet("font-weight: bold; color: rgb(0, 130, 0);");
        resultLabel->setText("Current channel span calibrated, correction "
                             + QString::number(calData->currentCorrection, 'f', 5));
        appendLog("Current span OK over " + QString::number(loadRequested.size()) + " points, scale "
                  + QString::number(scale, 'f', 5));

        loadDisabled = true;
        goToStep(AUTOCAL_STEP_CURRENT_ZERO2);
        return;
    }

    loadMeasured.append(mean);
    appendLog("Load point " + QString::number(loadMeasured.size()) + ": requested "
              + QString::number((int)loadRequested[loadPhase]) + " mA, measured "
              + QString::number(mean, 'f', 1) + " mA");

    loadPhase++;
    loadSettleTicks = 0;
    currentHistory.clear();

    if(loadPhase < loadRequested.size())
    {
        emit sigSetLoadCurrent((int)loadRequested[loadPhase]);
        return;
    }

    loadTimer->stop();
    emit sigSetLoadEnabled(false);
    emit sigSetLoadCurrent(0);

    /*Least squares line through all measured points, measured = slope*requested + b*/
    int n = loadRequested.size();
    double sx = 0, sy = 0, sxx = 0, sxy = 0;

    for(int i = 0; i < n; i++)
    {
        sx += loadRequested[i];
        sy += loadMeasured[i];
        sxx += loadRequested[i] * loadRequested[i];
        sxy += loadRequested[i] * loadMeasured[i];
    }

    double denom = n * sxx - sx * sx;

    if(fabs(denom) < 1e-9)
    {
        failAndRestart("Load calibration could not fit the measured points.");
        return;
    }

    double slope = (n * sxy - sx * sy) / denom;
    double intercept = (sy - slope * sx) / n;

    if(slope <= 0.0001)
    {
        failAndRestart("Load did not respond to the requested current.");
        return;
    }

    double oldCor = (calData->dacCorrection > 0) ? calData->dacCorrection : 1.0;
    double oldOff = calData->dacOffset;

    /*Firmware drives the DAC with requested*cor+off, so the correction and offset
      that make the measured current follow the requested one are derived from the
      measured slope and intercept and the parameters already in the device*/
    calData->dacCorrection = oldCor / slope;
    calData->dacOffset = oldOff - intercept * oldCor / slope;

    appendLog("Load fit over " + QString::number(n) + " points: slope "
              + QString::number(slope, 'f', 4) + ", intercept " + QString::number(intercept, 'f', 2) + " mA");
    resultLabel->setStyleSheet("font-weight: bold; color: rgb(0, 130, 0);");
    resultLabel->setText("Load calibrated, correction " + QString::number(calData->dacCorrection, 'f', 4)
                         + ", offset " + QString::number(calData->dacOffset, 'f', 2) + " mA");
    appendLog("Load channel OK, correction " + QString::number(calData->dacCorrection, 'f', 4)
              + ", offset " + QString::number(calData->dacOffset, 'f', 2) + " mA");

    goToStep(AUTOCAL_STEP_DONE);
}

void AutoCalibrationWnd::onNextClicked()
{
    if(step == AUTOCAL_STEP_DONE)
    {
        emit sigCalibrationFinished(true);
        close();
    }
}

void AutoCalibrationWnd::onBackClicked()
{
    if(step <= AUTOCAL_STEP_DISCONNECT_INPUT) return;
    goToStep((autocal_step_t)(step - 1));
}

void AutoCalibrationWnd::onReadyClicked()
{
    appendLog("User confirmed the step manually");

    switch(step)
    {
    case AUTOCAL_STEP_DISCONNECT_INPUT: goToStep(AUTOCAL_STEP_SET_REFERENCE);  break;
    case AUTOCAL_STEP_SET_REFERENCE:    goToStep(AUTOCAL_STEP_VOLTAGE_TUNE);   break;
    case AUTOCAL_STEP_JUMPER_BATTERY:   goToStep(AUTOCAL_STEP_CONNECT_SOURCE); break;
    case AUTOCAL_STEP_CONNECT_SOURCE:   goToStep(AUTOCAL_STEP_LOAD_TUNE);      break;
    default: break;
    }
}

void AutoCalibrationWnd::onCancelClicked()
{
    loadTimer->stop();
    emit sigSetLoadEnabled(false);
    emit sigSetLoadCurrent(0);
    step = AUTOCAL_STEP_IDLE;
    emit sigCalibrationFinished(false);
    close();
}
