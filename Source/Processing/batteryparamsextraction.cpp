#include "batteryparamsextraction.h"

#include <QList>
#include <math.h>

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
    defaults.tauRefinementNo = BATTERYPARAMS_TAU_REFINEMENT_NO;
    defaults.tauSeparation = BATTERYPARAMS_TAU_SEPARATION;
    defaults.capacity = 0;
    defaults.initialSoc = BATTERYPARAMS_INITIAL_SOC_DEFAULT;
    defaults.plotDecimation = BATTERYPARAMS_CYCLE_PLOT_DECIMATION_DEFAULT;

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

/*With the time constants fixed the relaxation curve is linear in its remaining
  parameters, so the open circuit voltage the cell is heading to and the branch
  amplitudes are obtained by least squares. Fitting the asymptote instead of
  taking it from the last sample keeps the result usable even when the pause is
  cut short*/
static bool prvBATTERYPARAMS_RelaxationFit(const QVector<double> &time, const QVector<double> &voltage,
                                           double tauFast, double tauSlow, bool secondOrder,
                                           double *openCircuitVoltage, double *amplitudeFast, double *amplitudeSlow)
{
    int size = secondOrder ? 3 : 2;
    double matrix[3][4];

    for(int row = 0; row < size; row++)
    {
        for(int column = 0; column < size + 1; column++)
        {
            matrix[row][column] = 0;
        }
    }

    for(int i = 0; i < time.size(); i++)
    {
        double basis[3];

        basis[0] = 1.0;
        basis[1] = exp(-time[i] / tauFast);
        basis[2] = secondOrder ? exp(-time[i] / tauSlow) : 0;

        for(int row = 0; row < size; row++)
        {
            for(int column = 0; column < size; column++)
            {
                matrix[row][column] += basis[row] * basis[column];
            }
            matrix[row][size] += basis[row] * voltage[i];
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

        for(int i = 0; i < size + 1; i++)
        {
            double temporary = matrix[column][i];
            matrix[column][i] = matrix[pivot][i];
            matrix[pivot][i] = temporary;
        }

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

    *openCircuitVoltage = matrix[0][size] / matrix[0][0];
    *amplitudeFast = -matrix[1][size] / matrix[1][1];
    *amplitudeSlow = secondOrder ? (-matrix[2][size] / matrix[2][2]) : 0;

    return true;
}

bool BatteryParamsExtraction::analyzeCycle(batteryparams_cycle_t *cycle, batteryparams_settings_t settings)
{
    batteryparams_model_t model = settings.model;
    int pauseStart = -1;
    int pauseEnd = -1;
    int relaxPosition;
    int fastPosition;
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
    cycle->tauFast = 0;
    cycle->tauSlow = 0;
    cycle->r1 = 0;
    cycle->c1 = 0;
    cycle->r2 = 0;
    cycle->c2 = 0;
    cycle->fitRmse = 0;
    cycle->fitMaxError = 0;
    cycle->fitKeys.clear();
    cycle->fitVoltage.clear();

    for(int i = 0; i < cycle->plotKeys.size(); i++)
    {
        if((pauseStart < 0) && (cycle->plotKeys[i] >= cycle->pauseStartKey)) pauseStart = i;
        if(cycle->plotKeys[i] <= cycle->pauseEndKey) pauseEnd = i;
    }

    if((pauseStart < 0) || (pauseEnd <= pauseStart)) return false;

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

    if((searchStart < 0) || (searchEnd <= searchStart)) return false;

    if(searchKeys.size() != searchVoltage.size()) return false;

    seriesSmooth(searchKeys, searchVoltage, settings.relaxationFilter, &smoothedVoltage);

    if(smoothedVoltage.size() != searchKeys.size()) return false;

    relaxPosition = prvBATTERYPARAMS_RelaxationPositionGet(searchKeys, smoothedVoltage,
                                                           searchStart, searchEnd, threshold, window);

    if(relaxPosition < 0) return false;

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

    /*Cell is at rest once it relaxed, so the voltage measured there is its open
      circuit voltage*/
    cycle->ocv = cycle->relaxationVoltage;

    /*Overpotential left on the RC branches when the pulse ends, decaying to zero
      as the cell relaxes towards its open circuit voltage*/
    /*Parameters are taken either from the part of the pause up to the point where
      the cell relaxed, or from the whole pause. Errors and the drawn curve always
      cover the whole pause, so the two settings stay comparable*/
    int fitEndPosition = (settings.fitEnd == BATTERYPARAMS_FIT_END_RELAXATION) ? relaxPosition : pauseEnd;

    for(int i = pauseStart; i <= fitEndPosition; i++)
    {
        relaxTime.append((cycle->plotKeys[i] - cycle->pauseStartKey) / 1000.0);
        relaxValue.append(cycle->plotVoltage[i]);
    }

    if(relaxTime.size() < 4) return false;

    /*Time constants are searched for on a logarithmic grid, with the asymptote
      and the amplitudes solved by least squares for every candidate. The grid is
      then narrowed around the best pair a few times to refine the result.
      Settling takes about five time constants, so nothing much longer than the
      relaxation itself is worth trying*/
    {
        double relaxDuration = relaxTime.last();
        double tauFastLow = relaxTime[1] - relaxTime[0];
        double tauFastHigh = relaxDuration / BATTERYPARAMS_SETTLING_TAU_NO * 2.0;
        double tauSlowLow;
        double tauSlowHigh;
        double bestError = -1;

        if(tauFastLow <= 0) tauFastLow = relaxDuration / 1000.0;
        if(tauFastHigh <= tauFastLow) tauFastHigh = tauFastLow * 10.0;

        tauSlowLow = tauFastLow;
        tauSlowHigh = tauFastHigh;

        for(int refinement = 0; refinement < settings.tauRefinementNo; refinement++)
        {
            double bestTauFast = 0;
            double bestTauSlow = 0;
            double bestAmplitudeFast = 0;
            double bestAmplitudeSlow = 0;
            double bestVoltage = 0;

            bestError = -1;

            for(int fastStep = 0; fastStep < settings.tauGridPoints; fastStep++)
            {
                double tauFastCandidate = tauFastLow * pow(tauFastHigh / tauFastLow,
                                                           (double)fastStep / (settings.tauGridPoints - 1));
                int slowSteps = secondOrder ? settings.tauGridPoints : 1;

                for(int slowStep = 0; slowStep < slowSteps; slowStep++)
                {
                    double tauSlowCandidate = 0;
                    double candidateFast = 0;
                    double candidateSlow = 0;
                    double candidateVoltage = 0;
                    double errorSum = 0;

                    if(secondOrder)
                    {
                        tauSlowCandidate = tauSlowLow * pow(tauSlowHigh / tauSlowLow,
                                                            (double)slowStep / (settings.tauGridPoints - 1));

                        /*Branches have to be clearly separated, otherwise the two
                          exponentials describe the same thing*/
                        if(tauSlowCandidate < tauFastCandidate * settings.tauSeparation) continue;
                    }

                    if(!prvBATTERYPARAMS_RelaxationFit(relaxTime, relaxValue, tauFastCandidate, tauSlowCandidate,
                                                       secondOrder, &candidateVoltage, &candidateFast, &candidateSlow))
                    {
                        continue;
                    }

                    if(candidateFast < 0) continue;
                    if(secondOrder && (candidateSlow < 0)) continue;

                    for(int i = 0; i < relaxTime.size(); i++)
                    {
                        double fitted = candidateVoltage - candidateFast * exp(-relaxTime[i] / tauFastCandidate);
                        double error;

                        if(secondOrder) fitted -= candidateSlow * exp(-relaxTime[i] / tauSlowCandidate);

                        error = relaxValue[i] - fitted;
                        errorSum += error * error;
                    }

                    if((bestError < 0) || (errorSum < bestError))
                    {
                        bestError = errorSum;
                        bestTauFast = tauFastCandidate;
                        bestTauSlow = tauSlowCandidate;
                        bestAmplitudeFast = candidateFast;
                        bestAmplitudeSlow = candidateSlow;
                        bestVoltage = candidateVoltage;
                    }
                }
            }

            if(bestError < 0) return false;

            cycle->tauFast = bestTauFast;
            cycle->tauSlow = bestTauSlow;
            amplitudeFast = bestAmplitudeFast;
            amplitudeSlow = bestAmplitudeSlow;
            openCircuitVoltage = bestVoltage;

            tauFastLow = bestTauFast / BATTERYPARAMS_TAU_REFINEMENT_SPAN;
            tauFastHigh = bestTauFast * BATTERYPARAMS_TAU_REFINEMENT_SPAN;

            if(secondOrder)
            {
                tauSlowLow = bestTauSlow / BATTERYPARAMS_TAU_REFINEMENT_SPAN;
                tauSlowHigh = bestTauSlow * BATTERYPARAMS_TAU_REFINEMENT_SPAN;
            }
        }
    }

    if(secondOrder && (cycle->tauSlow <= cycle->tauFast))
    {
        secondOrder = false;
        cycle->model = BATTERYPARAMS_MODEL_FIRST_ORDER;
        cycle->tauSlow = 0;
        amplitudeSlow = 0;
    }

    /*Overpotential left on the RC branches when the pulse ends, decaying to zero
      as the cell relaxes towards its open circuit voltage*/
    pulseDuration = (cycle->pulseEndKey - cycle->pulseStartKey) / 1000.0;
    pulseCurrent = cycle->pulseEndCurrent / 1000.0;

    if((pulseDuration <= 0) || (fabs(pulseCurrent) < 1e-9)) return false;

    /*During the pulse each branch charges towards I*R, so the amplitude seen at
      the beginning of the relaxation is scaled by how far it got*/
    double chargedFast = 1.0 - exp(-pulseDuration / cycle->tauFast);

    if(chargedFast <= 0) return false;

    cycle->r1 = amplitudeFast / (pulseCurrent * chargedFast);
    cycle->c1 = (fabs(cycle->r1) > 1e-9) ? cycle->tauFast / cycle->r1 : 0;

    if(secondOrder)
    {
        double chargedSlow = 1.0 - exp(-pulseDuration / cycle->tauSlow);

        if(chargedSlow <= 0) return false;

        cycle->r2 = amplitudeSlow / (pulseCurrent * chargedSlow);
        cycle->c2 = (fabs(cycle->r2) > 1e-9) ? cycle->tauSlow / cycle->r2 : 0;
    }

    double squareErrorSum = 0;
    int errorSampleNo = 0;

    /*Evaluated over the whole pause no matter where the parameters came from*/
    for(int i = pauseStart; i <= pauseEnd; i++)
    {
        double time = (cycle->plotKeys[i] - cycle->pauseStartKey) / 1000.0;
        double fitted = openCircuitVoltage - amplitudeFast * exp(-time / cycle->tauFast);
        double error;

        if(secondOrder)
        {
            fitted -= amplitudeSlow * exp(-time / cycle->tauSlow);
        }

        error = cycle->plotVoltage[i] - fitted;

        squareErrorSum += error * error;
        errorSampleNo++;
        if(fabs(error) > cycle->fitMaxError) cycle->fitMaxError = fabs(error);

        cycle->fitKeys.append(cycle->plotKeys[i]);
        cycle->fitVoltage.append(fitted);
    }

    if(errorSampleNo == 0) return false;

    cycle->fitRmse = sqrt(squareErrorSum / errorSampleNo);
    cycle->modelValid = true;

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
    activeCycle.relaxationReached = false;
    activeCycle.relaxationKey = 0;
    activeCycle.relaxationVoltage = 0;
    activeCycle.ocv = 0;
    activeCycle.model = settings.model;
    activeCycle.modelValid = false;
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

        if(samplingPeriod <= 0) return;
        if(keySamples.isEmpty()) return;

        markerKey = pendingMarkers.first().key * samplingPeriod;

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
