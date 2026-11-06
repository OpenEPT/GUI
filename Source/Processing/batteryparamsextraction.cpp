#include "batteryparamsextraction.h"

#include <QList>
#include <QThread>
#include <math.h>

#if defined(Q_OS_LINUX)
#include <pthread.h>
#endif

void BatteryParamsExtraction::nameCurrentThread(unsigned int cycleIndex)
{
    QThread::currentThread()->setObjectName(QString("OpenEPT BA (Cycle %1)").arg(cycleIndex));

#if defined(Q_OS_LINUX)
    /*The OS thread name htop shows is limited to 15 characters, so a compact name is
      used there while the full name stays on the Qt object*/
    QString shortName = QString("BA Cyc %1").arg(cycleIndex);
    if(shortName.size() > 15) shortName = shortName.left(15);
    pthread_setname_np(pthread_self(), shortName.toLocal8Bit().constData());
#endif
}

BatteryParamsExtraction::BatteryParamsExtraction(QObject *parent)
    : QObject{parent}
{
    pulseStartMarker = BATTERYPARAMS_DEFAULT_PULSE_START_MARKER;
    pulseEndMarker   = BATTERYPARAMS_DEFAULT_PULSE_END_MARKER;
    pauseStartMarker = BATTERYPARAMS_DEFAULT_PAUSE_START_MARKER;
    pauseEndMarker   = BATTERYPARAMS_DEFAULT_PAUSE_END_MARKER;

    samplingPeriod = 0;
    settings = settingsDefault();
    analysisDeferred = false;
    captureIncomplete = false;
    chargeTotal = 0;
    cycleActive = false;
    cycleCounter = 0;
    cycleReset();
}

void BatteryParamsExtraction::setMarkerNames(QString pulseStart, QString pulseEnd, QString pauseStart, QString pauseEnd)
{
    pulseStartMarker = pulseStart;
    pulseEndMarker   = pulseEnd;
    pauseStartMarker = pauseStart;
    pauseEndMarker   = pauseEnd;
}

void BatteryParamsExtraction::setSamplingPeriod(double aSamplingPeriod)
{
    samplingPeriod = aSamplingPeriod;
}

batteryparams_settings_t BatteryParamsExtraction::settingsDefault()
{
    batteryparams_settings_t defaults;

    defaults.relaxationThreshold = BATTERYPARAMS_RELAX_THRESHOLD_DEFAULT;
    defaults.relaxationWindow = BATTERYPARAMS_RELAX_WINDOW_DEFAULT;
    defaults.relaxationFilter = BATTERYPARAMS_RELAX_FILTER_DEFAULT;
    defaults.model = BATTERYPARAMS_MODEL_SECOND_ORDER;
    defaults.fitEnd = BATTERYPARAMS_FIT_END_PAUSE;
    defaults.tauGridPoints = BATTERYPARAMS_TAU_GRID_POINTS;
    defaults.tauSeparation = BATTERYPARAMS_TAU_SEPARATION;
    defaults.capacity = 0;
    defaults.initialSoc = BATTERYPARAMS_INITIAL_SOC_DEFAULT;
    defaults.plotDecimation = BATTERYPARAMS_CYCLE_PLOT_DECIMATION_DEFAULT;
    defaults.ocvWindow = BATTERYPARAMS_OCV_WINDOW_DEFAULT;
    defaults.fitAverageWindow = BATTERYPARAMS_FIT_AVERAGE_WINDOW_DEFAULT;

    return defaults;
}

void BatteryParamsExtraction::setSettings(batteryparams_settings_t aSettings)
{
    settings = aSettings;
}

/*Cycle detection and the model fitting can be separated, so that a whole
  recording is first split into cycles and the cycles are then fitted together,
  in parallel if asked for*/
void BatteryParamsExtraction::setAnalysisDeferred(bool aDeferred)
{
    analysisDeferred = aDeferred;
}

void BatteryParamsExtraction::setCaptureIncomplete(bool aCapture)
{
    captureIncomplete = aCapture;
}

void BatteryParamsExtraction::finalizeIncompleteCycle(double pauseEndKey)
{
    if(!cycleActive) return;

    if(activeCycle.pulseStartKey <= 0 || keySamples.isEmpty())
    {
        cycleActive = false;
        return;
    }

    activeCycle.pauseEndKey = pauseEndKey;
    activeCycle.incomplete = true;
    activeCycle.modelReason = "Incomplete cycle: the pulse/pause markers were not all captured, no model fitted";
    storeCycleSamples();
    cycles.append(activeCycle);
    cycleActive = false;
    emit sigCycleFinished(activeCycle);
    trimSamples(pauseEndKey);
}

void BatteryParamsExtraction::flushIncompleteCycle()
{
    if(!cycleActive || keySamples.isEmpty()) return;
    finalizeIncompleteCycle(keySamples.last());
}

batteryparams_settings_t BatteryParamsExtraction::getSettings()
{
    return settings;
}

/*Battery is considered relaxed once the terminal voltage stops changing, which
  means the span between the highest and the lowest sample from that moment to
  the end of the pause stays below the threshold. Looking only at a sliding
  window is not enough: a slow exponential tail always has an early window that
  happens to change little while the voltage still has tens of millivolts to go,
  and a window wide drift accumulates unnoticed. The window is what the point
  still has to be away from the end of the pause, so the condition is backed by
  at least that much data*/
static int prvBATTERYPARAMS_RelaxationPositionGet(const QVector<double> &keys, const QVector<double> &voltage,
                                                  int start, int end, double threshold, double window)
{
    QVector<int> maximumPositions;
    QVector<int> minimumPositions;
    int first = start;

    if(end < start) return -1;

    /*Cell counts as relaxed at the first sample whose preceding window stayed inside
      the threshold. Running maximum and minimum of that window are held in two
      monotonic queues, so the whole pause is scanned once*/
    for(int i = start; i <= end; i++)
    {
        while(!maximumPositions.isEmpty() && (voltage[maximumPositions.last()] <= voltage[i])) maximumPositions.removeLast();
        maximumPositions.append(i);

        while(!minimumPositions.isEmpty() && (voltage[minimumPositions.last()] >= voltage[i])) minimumPositions.removeLast();
        minimumPositions.append(i);

        while((keys[i] - keys[first]) > window)
        {
            if(maximumPositions.first() == first) maximumPositions.removeFirst();
            if(minimumPositions.first() == first) minimumPositions.removeFirst();
            first++;
        }

        if((keys[i] - keys[start]) < window) continue;

        if((voltage[maximumPositions.first()] - voltage[minimumPositions.first()]) <= threshold) return i;
    }

    return -1;
}

/*Fits a single decaying exponential value ~ amplitude * exp(-time / tau) over the
  given index range. The time constant is searched on a logarithmic grid, the
  amplitude is solved by least squares for every candidate, and the grid is narrowed
  around the best value a few times. Used for peeling the slow and fast branches off
  separate portions of the relaxation*/
static bool prvBATTERYPARAMS_SingleExpFit(const QVector<double> &time, const QVector<double> &value,
                                          int startIndex, int endIndex,
                                          double tauLow, double tauHigh,
                                          int gridPoints, int refinementNo,
                                          double *bestTau, double *bestAmplitude)
{
    if((startIndex < 0) || (endIndex >= time.size()) || ((endIndex - startIndex) < 1)) return false;
    if(gridPoints < 2) gridPoints = 2;
    if(refinementNo < 1) refinementNo = 1;
    if(tauLow <= 0) return false;
    if(tauHigh <= tauLow) tauHigh = tauLow * 10.0;

    double low = tauLow;
    double high = tauHigh;
    double resultTau = 0;
    double resultAmplitude = 0;
    bool found = false;

    /*Sum of squares of the samples does not depend on tau, so it is computed once and
      the fit error is obtained algebraically instead of a second exp() pass per tau:
      error = sum(value^2) - amplitude * sum(value * basis)*/
    double sumValueSq = 0;
    for(int i = startIndex; i <= endIndex; i++) sumValueSq += value[i] * value[i];

    for(int refinement = 0; refinement < refinementNo; refinement++)
    {
        double refineBestError = -1;
        double refineBestTau = 0;
        double refineBestAmplitude = 0;

        for(int step = 0; step < gridPoints; step++)
        {
            double tau = low * pow(high / low, (double)step / (gridPoints - 1));
            double sumXe = 0;
            double sumEe = 0;

            for(int i = startIndex; i <= endIndex; i++)
            {
                double basis = exp(-time[i] / tau);
                sumXe += value[i] * basis;
                sumEe += basis * basis;
            }

            if(sumEe <= 0) continue;

            double amplitude = sumXe / sumEe;
            double error = sumValueSq - amplitude * sumXe;

            if((refineBestError < 0) || (error < refineBestError))
            {
                refineBestError = error;
                refineBestTau = tau;
                refineBestAmplitude = amplitude;
            }
        }

        if(refineBestError < 0) break;

        resultTau = refineBestTau;
        resultAmplitude = refineBestAmplitude;
        found = true;

        low = refineBestTau / BATTERYPARAMS_TAU_REFINEMENT_SPAN;
        high = refineBestTau * BATTERYPARAMS_TAU_REFINEMENT_SPAN;
    }

    if(!found) return false;

    *bestTau = resultTau;
    *bestAmplitude = resultAmplitude;
    return true;
}

bool BatteryParamsExtraction::analyzeCycle(batteryparams_cycle_t *cycle, batteryparams_settings_t settings,
                                           const std::function<void(const QString&)> &progress)
{
    batteryparams_model_t model = settings.model;
    int pauseStart = -1;
    int pauseEnd = -1;
    int relaxPosition;
    bool secondOrder = (model == BATTERYPARAMS_MODEL_SECOND_ORDER);
    double threshold = settings.relaxationThreshold / 1000.0;
    double window = settings.relaxationWindow * 1000.0;
    double pulseDuration;
    double pulseCurrent;
    double amplitudeFast = 0;
    double amplitudeSlow = 0;
    double openCircuitVoltage = 0;
    QVector<double> relaxTime;
    QVector<double> relaxValue;

    if(cycle == NULL) return false;

    cycle->relaxationReached = false;
    cycle->relaxationKey = 0;
    cycle->relaxationVoltage = 0;
    cycle->ocv = 0;
    cycle->model = model;
    cycle->modelValid = false;
    cycle->modelReason = "";
    cycle->tauFast = 0;
    cycle->tauSlow = 0;
    cycle->fastPortionEndKey = 0;
    cycle->fitEndKey = 0;
    cycle->r1 = 0;
    cycle->c1 = 0;
    cycle->r2 = 0;
    cycle->c2 = 0;
    cycle->r1Valid = false;
    cycle->r2Valid = false;
    cycle->fitRmse = 0;
    cycle->fitMaxError = 0;
    cycle->fitKeys.clear();
    cycle->fitVoltage.clear();
    cycle->avgKeys.clear();
    cycle->avgVoltage.clear();

    for(int i = 0; i < cycle->plotKeys.size(); i++)
    {
        if((pauseStart < 0) && (cycle->plotKeys[i] >= cycle->pauseStartKey)) pauseStart = i;
        if(cycle->plotKeys[i] <= cycle->pauseEndKey) pauseEnd = i;
    }

    if((pauseStart < 0) || (pauseEnd <= pauseStart)) { cycle->modelReason = "Pause markers missing or out of order, no samples between pause start and pause end"; return false; }

    /*The RC fit runs on a moving average of the pause, the window is set in points.
      A window of 1 keeps the raw samples. The average spans the whole pause and stays
      aligned to the plotted keys: the window only shifts at the two ends so that the
      pause start and the pause end are both inside their window and no offset is
      introduced by the averaging*/
    if(progress) progress("averaging the pause");

    int fitAvgWindow = (settings.fitAverageWindow > 1) ? settings.fitAverageWindow : 1;
    int fitAvgHalf = (fitAvgWindow - 1) / 2;
    QVector<double> pauseAverage = cycle->plotVoltage;

    /*The averaged trace drawn in the cycle window is capped, so a long pause does not add
      hundreds of thousands of points on top of the raw samples*/
    int pauseCount = pauseEnd - pauseStart + 1;
    int avgStride = (pauseCount > BATTERYPARAMS_FIT_MAX_POINTS) ? (pauseCount / BATTERYPARAMS_FIT_MAX_POINTS) : 1;

    for(int i = pauseStart; i <= pauseEnd; i++)
    {
        /*Centered window that shrinks towards the two ends: it is a single point at the
          pause start (so the first averaged point is exactly the pause start sample, with
          no offset), grows symmetrically to the full window in the middle, and shrinks
          back to a single point at the pause end*/
        int effHalf = fitAvgHalf;
        if((i - pauseStart) < effHalf) effHalf = i - pauseStart;
        if((pauseEnd - i) < effHalf) effHalf = pauseEnd - i;

        int lo = i - effHalf;
        int hi = i + effHalf;

        double sum = 0;
        int no = 0;
        for(int k = lo; k <= hi; k++) { sum += cycle->plotVoltage[k]; no++; }

        pauseAverage[i] = (no > 0) ? (sum / no) : cycle->plotVoltage[i];

        if((((i - pauseStart) % avgStride) == 0) || (i == pauseEnd))
        {
            cycle->avgKeys.append(cycle->plotKeys[i]);
            cycle->avgVoltage.append(pauseAverage[i]);
        }
    }

    /*Relaxation is searched over an evenly spaced series of averaged samples. The
      plotted waveform keeps the extremes of every bucket instead, and a time based
      filter over those uneven pairs does not give the same result as the filter the
      application runs on the live samples*/
    const QVector<double> &searchKeys = cycle->relaxKeys.isEmpty() ? cycle->plotKeys : cycle->relaxKeys;
    const QVector<double> &searchVoltage = cycle->relaxKeys.isEmpty() ? cycle->plotVoltage : cycle->relaxVoltage;
    QVector<double> smoothedVoltage;
    int searchStart = -1;
    int searchEnd = -1;

    for(int i = 0; i < searchKeys.size(); i++)
    {
        if((searchStart < 0) && (searchKeys[i] >= cycle->pauseStartKey)) searchStart = i;
        if(searchKeys[i] <= cycle->pauseEndKey) searchEnd = i;
    }

    if((searchStart < 0) || (searchEnd <= searchStart)) { cycle->modelReason = "No samples found inside the pause for the relaxation search"; return false; }

    if(searchKeys.size() != searchVoltage.size()) { cycle->modelReason = "Relaxation sample series is inconsistent (key and voltage lengths differ)"; return false; }

    if(progress) progress("searching the relaxation point");

    seriesSmooth(searchKeys, searchVoltage, settings.relaxationFilter, &smoothedVoltage);

    if(smoothedVoltage.size() != searchKeys.size()) { cycle->modelReason = "Relaxation smoothing failed"; return false; }

    relaxPosition = prvBATTERYPARAMS_RelaxationPositionGet(searchKeys, smoothedVoltage,
                                                           searchStart, searchEnd, threshold, window);

    if(relaxPosition >= 0)
    {
        cycle->relaxationReached = true;
        cycle->relaxationKey = searchKeys[relaxPosition];
        cycle->relaxationVoltage = searchVoltage[relaxPosition];

        /*Fit works on the plotted waveform, so the relaxation point is taken back to it*/
        relaxPosition = pauseEnd;

        for(int i = pauseStart; i <= pauseEnd; i++)
        {
            if(cycle->plotKeys[i] >= cycle->relaxationKey)
            {
                relaxPosition = i;
                break;
            }
        }
    }
    else
    {
        /*The relaxation point was not reached. Both fit intervals fall back to the pause
          end: the OCV and the model are then taken from the pause end instead of being
          dropped*/
        if(settings.fitEnd == BATTERYPARAMS_FIT_END_RELAXATION)
            cycle->modelReason = "Relaxation point not reached within the pause, OCV and the fit fall back to the pause end";
        relaxPosition = pauseEnd;
    }

    /*Overpotential left on the RC branches when the pulse ends, decaying to zero
      as the cell relaxes towards its open circuit voltage*/
    /*Parameters are taken either from the part of the pause up to the point where
      the cell relaxed, or from the whole pause. Errors and the drawn curve always
      cover the whole pause, so the two settings stay comparable*/
    int fitEndPosition = (settings.fitEnd == BATTERYPARAMS_FIT_END_RELAXATION) ? relaxPosition : pauseEnd;

    cycle->fitEndKey = cycle->plotKeys[fitEndPosition];

    /*Open circuit voltage is the average of the last points of the fit interval: the
      end is the relaxation point when the fit stops there, otherwise the pause end.
      The window width is set in points, averaging the settled tail keeps a noisy
      sample or the onset of the next pulse from pulling the value off*/
    {
        int ocvWindow = (settings.ocvWindow > 1) ? settings.ocvWindow : 1;
        int ocvStart = fitEndPosition - (ocvWindow - 1);
        if(ocvStart < pauseStart) ocvStart = pauseStart;

        double ocvSum = 0;
        int ocvNo = 0;
        for(int i = ocvStart; i <= fitEndPosition; i++)
        {
            ocvSum += pauseAverage[i];
            ocvNo++;
        }
        cycle->ocv = (ocvNo > 0) ? (ocvSum / ocvNo) : pauseAverage[fitEndPosition];
    }

    /*The exponential fit is over determined, so for a long pause only a capped number
      of evenly spaced points is fed to it. This keeps the grid search fast without
      changing the fitted time constants. The last point is always kept so the interval
      end is not lost*/
    int fitCount = fitEndPosition - pauseStart + 1;
    int fitStride = (fitCount > BATTERYPARAMS_FIT_MAX_POINTS) ? (fitCount / BATTERYPARAMS_FIT_MAX_POINTS) : 1;

    for(int i = pauseStart; i <= fitEndPosition; i += fitStride)
    {
        relaxTime.append((cycle->plotKeys[i] - cycle->pauseStartKey) / 1000.0);
        relaxValue.append(pauseAverage[i]);
    }

    if((fitStride > 1) && (((fitEndPosition - pauseStart) % fitStride) != 0))
    {
        relaxTime.append((cycle->plotKeys[fitEndPosition] - cycle->pauseStartKey) / 1000.0);
        relaxValue.append(pauseAverage[fitEndPosition]);
    }

    if(relaxTime.size() < 4) { cycle->modelReason = "Too few samples in the fit interval (need at least 4)"; return false; }

    if(progress) progress("fitting the RC time constants");

    /*The fit is anchored at both ends: the open circuit voltage is the relaxed
      voltage, and the branch amplitudes sum to the overpotential present at the
      pause start, so the drawn curve always starts exactly from the pause start
      point and settles on the open circuit voltage.
      The slow branch is peeled from the later portion of the relaxation, where the
      fast branch has already settled, and the fast branch is then fitted on the
      early residual, so it is clear which portion of the signal each time constant
      comes from*/
    openCircuitVoltage = cycle->ocv;

    double overpotential = openCircuitVoltage - cycle->pauseStartVoltage;

    if(fabs(overpotential) < 1e-6) { cycle->modelReason = "Open circuit voltage equals the pause start voltage, no overpotential to fit (the cell barely moved during the pulse)"; return false; }

    {
        int sampleNo = relaxTime.size();
        double relaxDuration = relaxTime.last();
        QVector<double> decay(sampleNo);

        for(int i = 0; i < sampleNo; i++)
        {
            decay[i] = openCircuitVoltage - relaxValue[i];
        }

        double sampleStep = relaxTime[1] - relaxTime[0];
        if(sampleStep <= 0) sampleStep = relaxDuration / sampleNo;

        int splitIndex = sampleNo - 1;
        for(int i = 0; i < sampleNo; i++)
        {
            if(relaxTime[i] >= relaxDuration * BATTERYPARAMS_PEEL_SPLIT)
            {
                splitIndex = i;
                break;
            }
        }
        if(splitIndex < 1) splitIndex = 1;
        if(splitIndex > sampleNo - 2) splitIndex = sampleNo - 2;

        double tauSlowValue = 0;
        double amplitudeSlowValue = 0;
        bool slowOk = secondOrder &&
                      prvBATTERYPARAMS_SingleExpFit(relaxTime, decay, splitIndex, sampleNo - 1,
                          sampleStep, relaxDuration, settings.tauGridPoints, BATTERYPARAMS_TAU_REFINEMENT_NO,
                          &tauSlowValue, &amplitudeSlowValue);

        double tauFastValue = 0;
        double amplitudeFastValue = 0;
        bool fastOk;

        if(slowOk)
        {
            QVector<double> residual(splitIndex);

            for(int i = 0; i < splitIndex; i++)
            {
                residual[i] = decay[i] - amplitudeSlowValue * exp(-relaxTime[i] / tauSlowValue);
            }

            fastOk = prvBATTERYPARAMS_SingleExpFit(relaxTime, residual, 0, splitIndex - 1,
                        sampleStep, tauSlowValue / settings.tauSeparation,
                        settings.tauGridPoints, BATTERYPARAMS_TAU_REFINEMENT_NO,
                        &tauFastValue, &amplitudeFastValue);
        }
        else
        {
            fastOk = prvBATTERYPARAMS_SingleExpFit(relaxTime, decay, 0, sampleNo - 1,
                        sampleStep, relaxDuration, settings.tauGridPoints, BATTERYPARAMS_TAU_REFINEMENT_NO,
                        &tauFastValue, &amplitudeFastValue);
            secondOrder = false;
        }

        (void)amplitudeFastValue;

        if(!fastOk) { cycle->modelReason = "Exponential fit of the fast branch did not converge"; return false; }

        if(secondOrder && (tauSlowValue <= tauFastValue))
        {
            secondOrder = false;
        }

        cycle->tauFast = tauFastValue;
        cycle->tauSlow = secondOrder ? tauSlowValue : 0;

        if(secondOrder)
        {
            /*Amplitudes are re-solved under the constraint that they sum to the
              overpotential, so the curve is pinned to the pause start voltage*/
            double numerator = 0;
            double denominator = 0;

            for(int i = 0; i < sampleNo; i++)
            {
                double basisFast = exp(-relaxTime[i] / cycle->tauFast);
                double basisSlow = exp(-relaxTime[i] / cycle->tauSlow);
                double difference = basisFast - basisSlow;

                numerator += (decay[i] - overpotential * basisSlow) * difference;
                denominator += difference * difference;
            }

            amplitudeFast = (denominator > 0) ? (numerator / denominator) : overpotential;

            if((overpotential > 0) && (amplitudeFast < 0)) amplitudeFast = 0;
            if((overpotential < 0) && (amplitudeFast > 0)) amplitudeFast = 0;
            if(fabs(amplitudeFast) > fabs(overpotential)) amplitudeFast = overpotential;

            amplitudeSlow = overpotential - amplitudeFast;
        }
        else
        {
            amplitudeFast = overpotential;
            amplitudeSlow = 0;
        }

        double boundaryTime = BATTERYPARAMS_PEEL_BOUNDARY_TAU * cycle->tauFast;

        if(secondOrder && (boundaryTime > relaxTime[splitIndex])) boundaryTime = relaxTime[splitIndex];
        if(boundaryTime > relaxDuration) boundaryTime = relaxDuration;

        cycle->fastPortionEndKey = cycle->pauseStartKey + boundaryTime * 1000.0;
    }

    cycle->model = secondOrder ? BATTERYPARAMS_MODEL_SECOND_ORDER : BATTERYPARAMS_MODEL_FIRST_ORDER;

    /*During the pulse each branch charges towards I*R, so the amplitude seen at the
      beginning of the relaxation is scaled by how far it got*/
    pulseDuration = (cycle->pulseEndKey - cycle->pulseStartKey) / 1000.0;
    pulseCurrent = cycle->pulseEndCurrent / 1000.0;

    if((pulseDuration <= 0) || (fabs(pulseCurrent) < 1e-9)) { cycle->modelReason = "Pulse duration or pulse current is zero, R1/C1 cannot be scaled"; return false; }

    double chargedFast = 1.0 - exp(-pulseDuration / cycle->tauFast);

    if(chargedFast <= 0) { cycle->modelReason = "Fast time constant too large for the pulse length, R1/C1 cannot be scaled"; return false; }

    /*A branch is only reported when it actually carries part of the relaxation. If its
      amplitude solved to (near) zero the branch could not be determined, so its R and C
      are left empty instead of shown as zero*/
    double amplitudeEps = 1e-4 * fabs(overpotential);

    cycle->r1 = amplitudeFast / (pulseCurrent * chargedFast);
    cycle->c1 = (fabs(cycle->r1) > 1e-9) ? cycle->tauFast / cycle->r1 : 0;
    cycle->r1Valid = (fabs(amplitudeFast) > amplitudeEps);

    if(secondOrder)
    {
        double chargedSlow = 1.0 - exp(-pulseDuration / cycle->tauSlow);

        if(chargedSlow <= 0) { cycle->modelReason = "Slow time constant too large for the pulse length, R2/C2 cannot be scaled"; return false; }

        cycle->r2 = amplitudeSlow / (pulseCurrent * chargedSlow);
        cycle->c2 = (fabs(cycle->r2) > 1e-9) ? cycle->tauSlow / cycle->r2 : 0;
        cycle->r2Valid = (fabs(amplitudeSlow) > amplitudeEps);
    }

    if(!cycle->r1Valid && cycle->r2Valid)
        cycle->modelReason = "Fast RC branch (R1/C1) could not be determined: it had no amplitude, only the slow branch (R2/C2) was identified";
    else if(cycle->r1Valid && secondOrder && !cycle->r2Valid)
        cycle->modelReason = "Slow RC branch (R2/C2) could not be determined: it had no amplitude, only the fast branch (R1/C1) was identified";

    if(progress) progress("evaluating the fit error");

    double squareErrorSum = 0;
    int errorSampleNo = 0;

    /*Evaluated over the whole pause no matter where the parameters came from. The drawn
      curve and the error are sampled at the same capped rate as the fit, so a long pause
      does not spend time evaluating exp() and drawing hundreds of thousands of points*/
    int errCount = pauseEnd - pauseStart + 1;
    int errStride = (errCount > BATTERYPARAMS_FIT_MAX_POINTS) ? (errCount / BATTERYPARAMS_FIT_MAX_POINTS) : 1;

    for(int i = pauseStart; i <= pauseEnd; i += errStride)
    {
        double time = (cycle->plotKeys[i] - cycle->pauseStartKey) / 1000.0;
        double fitted = openCircuitVoltage - amplitudeFast * exp(-time / cycle->tauFast);
        double error;

        if(secondOrder)
        {
            fitted -= amplitudeSlow * exp(-time / cycle->tauSlow);
        }

        error = pauseAverage[i] - fitted;

        squareErrorSum += error * error;
        errorSampleNo++;
        if(fabs(error) > cycle->fitMaxError) cycle->fitMaxError = fabs(error);

        cycle->fitKeys.append(cycle->plotKeys[i]);
        cycle->fitVoltage.append(fitted);
    }

    if(errorSampleNo == 0) { cycle->modelReason = "No samples to evaluate the fit error"; return false; }

    cycle->fitRmse = sqrt(squareErrorSum / errorSampleNo);
    cycle->modelValid = true;

    if(!secondOrder && cycle->modelReason.isEmpty()) cycle->modelReason = "First order model (single RC): R2/C2 not identified, the relaxation showed a single dominant time constant";

    return true;
}

QVector<batteryparams_cycle_t> BatteryParamsExtraction::getCycles()
{
    return cycles;
}

void BatteryParamsExtraction::cycleReset()
{
    activeCycle.index = 0;
    activeCycle.pulseStartKey = 0;
    activeCycle.pulseEndKey = 0;
    activeCycle.pauseStartKey = 0;
    activeCycle.pauseEndKey = 0;
    activeCycle.pulseEndVoltage = 0;
    activeCycle.pulseEndCurrent = 0;
    activeCycle.pauseStartVoltage = 0;
    activeCycle.pauseStartCurrent = 0;
    activeCycle.deltaVoltage = 0;
    activeCycle.deltaCurrent = 0;
    activeCycle.resistance = 0;
    activeCycle.charge = 0;
    activeCycle.chargeTotal = 0;
    activeCycle.socUsed = 0;
    activeCycle.soc = 0;
    activeCycle.resistanceValid = false;
    activeCycle.socValid = false;
    activeCycle.incomplete = false;
    activeCycle.relaxationReached = false;
    activeCycle.relaxationKey = 0;
    activeCycle.relaxationVoltage = 0;
    activeCycle.ocv = 0;
    activeCycle.model = settings.model;
    activeCycle.modelValid = false;
    activeCycle.modelReason = "";
    activeCycle.tauFast = 0;
    activeCycle.tauSlow = 0;
    activeCycle.r1 = 0;
    activeCycle.c1 = 0;
    activeCycle.r2 = 0;
    activeCycle.c2 = 0;
    activeCycle.fitRmse = 0;
    activeCycle.fitMaxError = 0;
    activeCycle.fitKeys.clear();
    activeCycle.fitVoltage.clear();
    activeCycle.plotKeys.clear();
    activeCycle.plotVoltage.clear();
    activeCycle.plotCurrent.clear();
    activeCycle.relaxKeys.clear();
    activeCycle.relaxVoltage.clear();
}

void BatteryParamsExtraction::storeCycleSamples()
{
    int startPosition = samplePositionGet(activeCycle.pulseStartKey);
    int endPosition = samplePositionGet(activeCycle.pauseEndKey);
    int samplesNo;
    int stride;
    double stepWindowStart = activeCycle.pulseEndKey - BATTERYPARAMS_CYCLE_PLOT_STEP_WINDOW;
    double stepWindowEnd = activeCycle.pauseStartKey + BATTERYPARAMS_CYCLE_PLOT_STEP_WINDOW;

    if(startPosition < 0 || endPosition < startPosition) return;

    samplesNo = endPosition - startPosition + 1;

    /*Charge taken out of the battery during the cycle, obtained by integrating
      the measured current. Sampling period is in ms and current in mA*/
    for(int i = startPosition; i < endPosition; i++)
    {
        activeCycle.charge += currentSamples[i] * (keySamples[i + 1] - keySamples[i]) / 3600000.0;
    }

    chargeTotal += activeCycle.charge;
    activeCycle.chargeTotal = chargeTotal;

    if(settings.capacity > 0)
    {
        activeCycle.socUsed = chargeTotal / settings.capacity * 100.0;
        activeCycle.soc = settings.initialSoc - activeCycle.socUsed;
        activeCycle.socValid = true;
    }

    /*Relaxation search needs evenly spaced averages that are not affected by how
      much the stored waveform is decimated. Buckets are kept well below the
      relaxation filter, otherwise averaging averages shifts the point*/
    int relaxStride = 1;

    if((settings.relaxationFilter > 0) && (samplingPeriod > 0))
    {
        relaxStride = (int)((settings.relaxationFilter / 20.0) / samplingPeriod);
    }

    if(relaxStride < 1) relaxStride = 1;

    for(int i = startPosition; i <= endPosition; i += relaxStride)
    {
        double keySum = 0;
        double voltageSum = 0;
        int no = 0;

        for(int k = i; (k <= endPosition) && (k < (i + relaxStride)); k++)
        {
            keySum += keySamples[k];
            voltageSum += voltageSamples[k];
            no++;
        }

        if(no == 0) continue;

        activeCycle.relaxKeys.append(keySum / (double)no);
        activeCycle.relaxVoltage.append(voltageSum / (double)no);
    }

    /*Stored waveform keeps every n-th sample, with the samples around the current
      step untouched because the step is what the resistance is calculated from*/
    stride = (settings.plotDecimation > 0) ? settings.plotDecimation : 1;

    for(int i = startPosition; i <= endPosition; i++)
    {
        bool inStepWindow = (keySamples[i] >= stepWindowStart) && (keySamples[i] <= stepWindowEnd);

        if(!inStepWindow && (((i - startPosition) % stride) != 0) && (i != endPosition)) continue;

        activeCycle.plotKeys.append(keySamples[i]);
        activeCycle.plotVoltage.append(voltageSamples[i]);
        activeCycle.plotCurrent.append(currentSamples[i]);
    }
}

int BatteryParamsExtraction::relaxationPositionGet(const QVector<double> &keys, const QVector<double> &voltage,
                                                   int start, int end, double threshold, double window)
{
    return prvBATTERYPARAMS_RelaxationPositionGet(keys, voltage, start, end, threshold, window);
}

/*Markers reported by the device can land on the current step itself instead of on
  the settled levels around it, and the resistance is then calculated across the
  step. Pulse End is moved to the last sample that still carries the pulse current
  and Pause Start to the first sample that already carries the pause current*/
bool BatteryParamsExtraction::cyclePointsFit(batteryparams_cycle_t *cycle)
{
    int startPosition = -1;
    int endPosition = -1;
    int stepPosition = -1;
    double stepDrop = 0;
    double pulseLevel = 0;
    double pauseLevel = 0;
    double amplitude;
    double tolerance;
    int pulseEndPosition;
    int pauseStartPosition;
    int no;

    if(cycle == NULL) return false;
    if(cycle->plotKeys.size() != cycle->plotCurrent.size()) return false;

    for(int i = 0; i < cycle->plotKeys.size(); i++)
    {
        if((startPosition < 0) && (cycle->plotKeys[i] >= cycle->pulseStartKey)) startPosition = i;
        if(cycle->plotKeys[i] <= cycle->pauseEndKey) endPosition = i;
    }

    if((startPosition < 0) || (endPosition <= startPosition)) return false;

    for(int i = startPosition; i < endPosition; i++)
    {
        double drop = cycle->plotCurrent[i] - cycle->plotCurrent[i + 1];

        if((stepPosition < 0) || (drop > stepDrop))
        {
            stepPosition = i;
            stepDrop = drop;
        }
    }

    if(stepPosition <= startPosition) return false;

    no = 0;
    for(int i = startPosition; i <= stepPosition; i++)
    {
        pulseLevel += cycle->plotCurrent[i];
        no++;
    }
    if(no == 0) return false;
    pulseLevel /= (double)no;

    no = 0;
    for(int i = stepPosition + 1; i <= endPosition; i++)
    {
        pauseLevel += cycle->plotCurrent[i];
        no++;
    }
    if(no == 0) return false;
    pauseLevel /= (double)no;

    amplitude = pulseLevel - pauseLevel;

    if(amplitude <= 0) return false;

    tolerance = amplitude * 0.1;

    pulseEndPosition = stepPosition;
    while((pulseEndPosition > startPosition) && (cycle->plotCurrent[pulseEndPosition] < (pulseLevel - tolerance)))
    {
        pulseEndPosition--;
    }

    pauseStartPosition = stepPosition + 1;
    while((pauseStartPosition < endPosition) && (cycle->plotCurrent[pauseStartPosition] > (pauseLevel + tolerance)))
    {
        pauseStartPosition++;
    }

    if((cycle->plotKeys[pulseEndPosition] == cycle->pulseEndKey) &&
       (cycle->plotKeys[pauseStartPosition] == cycle->pauseStartKey))
    {
        return false;
    }

    cycle->pulseEndKey = cycle->plotKeys[pulseEndPosition];
    cycle->pulseEndVoltage = cycle->plotVoltage[pulseEndPosition];
    cycle->pulseEndCurrent = cycle->plotCurrent[pulseEndPosition];

    cycle->pauseStartKey = cycle->plotKeys[pauseStartPosition];
    cycle->pauseStartVoltage = cycle->plotVoltage[pauseStartPosition];
    cycle->pauseStartCurrent = cycle->plotCurrent[pauseStartPosition];

    cycle->deltaVoltage = cycle->pauseStartVoltage - cycle->pulseEndVoltage;
    cycle->deltaCurrent = cycle->pulseEndCurrent - cycle->pauseStartCurrent;
    cycle->resistanceValid = fabs(cycle->deltaCurrent) > 0.000001;
    cycle->resistance = cycle->resistanceValid ? cycle->deltaVoltage / (cycle->deltaCurrent / 1000.0) : 0;

    return true;
}

/*Moves the pause start onto the sample at or after the given key and recomputes the
  series resistance from the new pause start. Pulse end stays where it is, so the
  instantaneous step the resistance is read from is measured against the new pause
  start. The RC fit is left to analyzeCycle, which reads the updated pause start*/
bool BatteryParamsExtraction::setPauseStart(batteryparams_cycle_t *cycle, double pauseStartKey)
{
    int position = -1;

    if(cycle == NULL) return false;
    if(cycle->plotKeys.size() != cycle->plotVoltage.size()) return false;
    if(cycle->plotKeys.size() != cycle->plotCurrent.size()) return false;

    for(int i = 0; i < cycle->plotKeys.size(); i++)
    {
        if(cycle->plotKeys[i] >= pauseStartKey)
        {
            position = i;
            break;
        }
    }

    if(position < 0) return false;
    if(cycle->plotKeys[position] <= cycle->pulseEndKey) return false;
    if(cycle->plotKeys[position] >= cycle->pauseEndKey) return false;

    cycle->pauseStartKey = cycle->plotKeys[position];
    cycle->pauseStartVoltage = cycle->plotVoltage[position];
    cycle->pauseStartCurrent = cycle->plotCurrent[position];

    cycle->deltaVoltage = cycle->pauseStartVoltage - cycle->pulseEndVoltage;
    cycle->deltaCurrent = cycle->pulseEndCurrent - cycle->pauseStartCurrent;
    cycle->resistanceValid = fabs(cycle->deltaCurrent) > 0.000001;
    cycle->resistance = cycle->resistanceValid ? cycle->deltaVoltage / (cycle->deltaCurrent / 1000.0) : 0;

    return true;
}

void BatteryParamsExtraction::seriesSmooth(const QVector<double> &keys, const QVector<double> &values,
                                           double window, QVector<double> *smoothed)
{
    double sum = 0;
    int first = 0;

    if(smoothed == NULL) return;

    smoothed->clear();

    if(keys.size() != values.size()) return;

    smoothed->reserve(values.size());

    /*Voltage noise is compared against a threshold of a few mV, so a single sample
      decides nothing. Every sample is replaced with the average of the samples that
      fall inside the filter window ending at it*/
    if(window <= 0)
    {
        *smoothed = values;
        return;
    }

    for(int i = 0; i < values.size(); i++)
    {
        sum += values[i];

        while((first < i) && ((keys[i] - keys[first]) > window))
        {
            sum -= values[first];
            first++;
        }

        smoothed->append(sum / (double)(i - first + 1));
    }
}

void BatteryParamsExtraction::onReset()
{
    voltageSamples.clear();
    currentSamples.clear();
    keySamples.clear();
    pendingMarkers.clear();
    cycles.clear();
    chargeTotal = 0;
    cycleActive = false;
    cycleCounter = 0;
    cycleReset();
}

void BatteryParamsExtraction::onNewSamplesReceived(QVector<double> voltage, QVector<double> current, QVector<double> voltageKeys, QVector<double> currentKeys)
{
    int samplesNo = voltage.size();

    Q_UNUSED(currentKeys);

    if(current.size() < samplesNo) samplesNo = current.size();
    if(voltageKeys.size() < samplesNo) samplesNo = voltageKeys.size();

    for(int i = 0; i < samplesNo; i++)
    {
        voltageSamples.append(voltage[i]);
        currentSamples.append(current[i]);
        keySamples.append(voltageKeys[i]);
    }

    /*Marker position is reported as a sample index counted from the acquisition
      start, while collected samples carry an absolute key. Sampling period is
      the link between the two and is taken from the samples themselves*/
    if((samplingPeriod <= 0) && (samplesNo > 1) && (voltageKeys[1] > voltageKeys[0]))
    {
        samplingPeriod = voltageKeys[1] - voltageKeys[0];
    }

    processPendingMarkers();
}

int BatteryParamsExtraction::samplePositionGet(double key)
{
    int low = 0;
    int high = keySamples.size() - 1;
    int position;

    if(keySamples.isEmpty()) return -1;

    while(low < high)
    {
        int middle = (low + high) / 2;
        if(keySamples[middle] < key)
        {
            low = middle + 1;
        }
        else
        {
            high = middle;
        }
    }

    position = low;

    /*Keys are sample times, so the closest one to the marker time is used*/
    if((position > 0) && (qAbs(keySamples[position - 1] - key) <= qAbs(keySamples[position] - key)))
    {
        position -= 1;
    }

    return position;
}

void BatteryParamsExtraction::trimSamples(double key)
{
    int position = samplePositionGet(key);

    if(position <= 0) return;

    voltageSamples.remove(0, position);
    currentSamples.remove(0, position);
    keySamples.remove(0, position);
}

void BatteryParamsExtraction::onNewMarkerReceived(double value, double key, QString name)
{
    batteryparams_marker_t marker;

    Q_UNUSED(value);

    if(key < 0) return;

    marker.name = name.trimmed();
    marker.key = key;

    pendingMarkers.append(marker);

    processPendingMarkers();
}

void BatteryParamsExtraction::processPendingMarkers()
{
    /*Markers and samples are produced by two independent links, so a marker can
      arrive before the samples it points to. Markers are processed in order and
      kept until the sample they reference is collected*/
    while(!pendingMarkers.isEmpty())
    {
        double markerKey;

        if(keySamples.isEmpty()) return;

        markerKey = pendingMarkers.first().key;

        if(markerKey > keySamples.last()) return;

        batteryparams_marker_t marker = pendingMarkers.takeFirst();

        /*Marker older than the collected samples cannot be resolved any more*/
        if(markerKey < keySamples.first()) continue;

        processMarker(marker.name, samplePositionGet(markerKey));
    }
}

void BatteryParamsExtraction::processMarker(QString name, int position)
{
    double voltage;
    double current;
    double key;

    if(position < 0 || position >= keySamples.size()) return;

    voltage = voltageSamples[position];
    current = currentSamples[position];
    key = keySamples[position];

    if(name == pulseStartMarker)
    {
        if(captureIncomplete && cycleActive) finalizeIncompleteCycle(key);
        cycleReset();
        cycleActive = true;
        cycleCounter++;
        activeCycle.index = cycleCounter;
        activeCycle.pulseStartKey = key;
        emit sigCycleStarted(activeCycle.index, key);
        return;
    }

    if(!cycleActive) return;

    if(name == pulseEndMarker)
    {
        activeCycle.pulseEndKey = key;
        activeCycle.pulseEndVoltage = voltage;
        activeCycle.pulseEndCurrent = current;
        return;
    }

    if(name == pauseStartMarker)
    {
        activeCycle.pauseStartKey = key;
        activeCycle.pauseStartVoltage = voltage;
        activeCycle.pauseStartCurrent = current;

        /*Internal resistance is obtained from the current step between the end of
          the pulse and the beginning of the pause. Voltage is in V, current in mA*/
        activeCycle.deltaVoltage = activeCycle.pauseStartVoltage - activeCycle.pulseEndVoltage;
        activeCycle.deltaCurrent = activeCycle.pulseEndCurrent - activeCycle.pauseStartCurrent;

        if(fabs(activeCycle.deltaCurrent) > 0.000001)
        {
            activeCycle.resistance = activeCycle.deltaVoltage / (activeCycle.deltaCurrent / 1000.0);
            activeCycle.resistanceValid = true;
        }
        return;
    }

    if(name == pauseEndMarker)
    {
        activeCycle.pauseEndKey = key;
        storeCycleSamples();
        if(!analysisDeferred) analyzeCycle(&activeCycle, settings);
        cycles.append(activeCycle);
        cycleActive = false;
        emit sigCycleFinished(activeCycle);
        trimSamples(key);
        return;
    }
}
