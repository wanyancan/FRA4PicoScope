//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frequency Response Analyzer for PicoScope
//
// Copyright (c) 2014 by Aaron Hexamer
//
// This file is part of the Frequency Response Analyzer for PicoScope program.
//
// Frequency Response Analyzer for PicoScope is free software: you can 
// redistribute it and/or modify it under the terms of the GNU General Public 
// License as published by the Free Software Foundation, either version 3 of 
// the License, or (at your option) any later version.
//
// Frequency Response Analyzer for PicoScope is distributed in the hope that 
// it will be useful,but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Frequency Response Analyzer for PicoScope.  If not, see <http://www.gnu.org/licenses/>.
//
// Module: PicoScopeFRA.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PicoScopeFRA.h"
#include "picoStatus.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <complex>
#include <algorithm>
#include <memory>
#include <vector>
#include <sstream>
#include <iomanip>
#include "plplot.h" // For creating diagnostic graphs

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: DataReady
//
// Purpose: Callback used to signal when data capture has been completed by the scope.
//
// Parameters: See PicoScope programming API
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void __stdcall DataReady( short handle, PICO_STATUS status, void * pParameter)
{
    if (PICO_OK == status)
    {
        SetEvent( *(HANDLE*)pParameter );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::PicoScopeFRA
//
// Purpose: Constructor
//
// Parameters: [in] statusCB - status callback
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

PicoScopeFRA::PicoScopeFRA(FRA_STATUS_CALLBACK statusCB)
{
    StatusCallback = statusCB;

    currentInputChannelRange = (PS_RANGE)0;
    currentOutputChannelRange = (PS_RANGE)0;

    mSamplingMode = LOW_NOISE;

    numSteps = latestCompletedNumSteps = freqStepCounter = 0;

    ovIn = ovOut = false;
    delayForAcCoupling = false;
    inputChannelAutorangeStatus = outputChannelAutorangeStatus = OK;

    hCaptureEvent = CreateEventW( NULL, false, false, L"CaptureEvent" );

    if ((HANDLE)NULL == hCaptureEvent)
    {
        throw runtime_error( "Failed to create Capture Event" );
    }

    mInputDcOffset = 0.0;
    mOutputDcOffset = 0.0;
    actualSampFreqHz = 0.0;
    numSamples = 0;
    timeIndisposedMs = 0;
    currentInputMagnitude = 0.0;
    currentOutputMagnitude = 0.0;
    currentInputPhase = 0.0;
    currentOutputPhase = 0.0;
    currentInputPurity = 0.0;
    currentOutputPurity = 0.0;
    mPurityLowerLimit = 0.0;
    minAllowedAmplitudeRatio = 0.0;
    minAmplitudeRatioTolerance = 0.0;
    maxAmplitudeRatio = 0.0;
    maxAutorangeRetries = 0;
    mExtraSettlingTimeMs = 0;
    mMinCyclesCaptured = 0;
    fSampNoiseRejectMode = 0.0;
    timebaseNoiseRejectMode = 0;
    rangeCounts = 0.0;
    signalGeneratorPrecision = 0.0;
    autorangeRetryCounter = 0;
    mDiagnosticsOn = false;
    rangeInfo = NULL;
    minRange = 0;
    maxRange = 0;
    ps = NULL;
    numChannels = 2;
    maxScopeSamplesPerChannel = 0;
    currentFreqHz = 0.0;
    currentOutputVolts = 0.0;
    mStartFreqHz = 0.0;
    mStopFreqHz = 0.0;
    mStepsPerDecade = 10;
    mInputChannel = PS_CHANNEL_A;
    mOutputChannel = PS_CHANNEL_B;
    mInputChannelCoupling = PS_AC;
    mOutputChannelCoupling = PS_AC;

    cancel = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::SetInstrument
//
// Purpose: Tells the FRA object which scope to use.
//
// Parameters: _ps - The scope to use
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::SetInstrument( PicoScope* _ps )
{
    ps = _ps;
    numChannels = ps->GetNumChannels();
    rangeInfo = ps->GetRangeCaps();
    fSampNoiseRejectMode = ps->GetNoiseRejectModeSampleRate();
    timebaseNoiseRejectMode = ps->GetNoiseRejectModeTimebase();
    signalGeneratorPrecision = ps->GetSignalGeneratorPrecision();
    minRange = ps->GetMinRange();
    maxRange = ps->GetMaxRange();
    rangeCounts = ps->GetMaxValue();

    // Setup arbitrary initial settings to force a calculation of maxScopeSamplesPerChannel
    SetupChannels( PS_CHANNEL_A, PS_AC, ATTEN_1X, 0.0, PS_CHANNEL_B, PS_AC, ATTEN_1X, 0.0, 0.0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::SetFraSettings
//
// Purpose: Provides configuration parameters for the FRA
//
// Parameters: [in] samplingMode - Low or high noise sampling mode
//             [in] purityLowerLimit - Lower limit on purity before we take action
//             [in] extraSettlingTimeMs - additional settling time to insert between setting up 
//                                        signal generator and sampling
//             [in] autorangeTriesPerStep - Number of range tries allowed
//             [in] autorangeTolerance - Hysterysis used to determine when the switch
//             [in] smallSignalResolutionTolerance - Lower limit on signal amplitide before we 
//                                                   take aciton
//             [in] maxAutorangeAmplitude - Amplitude before we switch to next higher range.
//             [in] minCyclesCaptured - Minimum cycles captured for stmulus signal
//             [in] diagnosticsOn - Whether to output plots of time domain data
//             [in] baseDataPath - Path providing location to store time domain plots
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::SetFraSettings( SamplingMode_T samplingMode, double purityLowerLimit, uint16_t extraSettlingTimeMs, uint8_t autorangeTriesPerStep,
                                   double autorangeTolerance, double smallSignalResolutionTolerance, double maxAutorangeAmplitude, uint16_t minCyclesCaptured,
                                   bool diagnosticsOn, wstring baseDataPath )
{
    char path[MAX_PATH];
    mSamplingMode = samplingMode;
    mPurityLowerLimit = purityLowerLimit;
    mExtraSettlingTimeMs = extraSettlingTimeMs;
    maxAutorangeRetries = autorangeTriesPerStep;
    minAmplitudeRatioTolerance = autorangeTolerance;
    minAllowedAmplitudeRatio = smallSignalResolutionTolerance;
    maxAmplitudeRatio = maxAutorangeAmplitude;
    mMinCyclesCaptured = minCyclesCaptured;
    mDiagnosticsOn = diagnosticsOn;
    WideCharToMultiByte( CP_UTF8, 0, baseDataPath.c_str(), -1, path, sizeof(path), NULL, NULL );
    mBaseDataPath = path;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::GetMinFrequency
//
// Purpose: Gets the minimum frequency that can be supported with the given sampling rate and 
//          buffer space
//
// Parameters: [out] return - Min Frequency supported
//
// Notes: Used by input parameter validation
//
///////////////////////////////////////////////////////////////////////////////////////////////////

double PicoScopeFRA::GetMinFrequency(void)
{
    if (mSamplingMode == LOW_NOISE)
    {
        return (ps->GetSignalGeneratorPrecision());
    }
    else
    {
        // Add in half the signal generator precision because the frequency could get rounded down
        return (((ps->GetSignalGeneratorPrecision())/2.0) + ((double)mMinCyclesCaptured * ( fSampNoiseRejectMode / (double)maxScopeSamplesPerChannel )));
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::SetupChannels
//
// Purpose: Sets up channel parameters for input channel, output channel, and function generator
//
// Parameters: [in] inputChannel - Channel to use for input signal
//             [in] inputChannelCoupling - AC/DC coupling for input channel
//             [in] inputChannelAttenuation - Attenuation setting for input channel
//             [in] inputDcOffset - DC Offset for input channel
//             [in] outputChannel - Channel to use for output signal
//             [in] outputChannelCoupling - AC/DC coupling for output channel
//             [in] outputChannelAttenuation - Attenuation setting for output channel
//             [in] outputDcOffset - DC Offset for output channel
//             [in] signalVpp - Volts peak to peak of the stimulus signal
//             [out] return - Whether the function was successful.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::SetupChannels( int inputChannel, int inputChannelCoupling, int inputChannelAttenuation, double inputDcOffset,
                                  int outputChannel, int outputChannelCoupling, int outputChannelAttenuation, double outputDcOffset,
                                  double signalVpp )
{
    FRA_STATUS_MESSAGE_T fraStatusMsg;

    currentInputChannelRange = minRange;
    currentOutputChannelRange = minRange;

    mInputChannel = (PS_CHANNEL)inputChannel;
    mOutputChannel = (PS_CHANNEL)outputChannel;

    mInputChannelCoupling = (PS_COUPLING)inputChannelCoupling;
    mOutputChannelCoupling = (PS_COUPLING)outputChannelCoupling;

    mInputDcOffset = inputDcOffset;
    mOutputDcOffset = outputDcOffset;

    currentOutputVolts = signalVpp;

    if (!(ps->Initialized()))
    {
        UpdateStatus( fraStatusMsg, FRA_STATUS_FATAL_ERROR, L"Fatal error: Device not initialized." );
        return false;
    }

    if( !(ps->SetupChannel((PS_CHANNEL)mInputChannel, (PS_COUPLING)mInputChannelCoupling, currentInputChannelRange, (float)mInputDcOffset)) )
    {
        return false;
    }
    if( !(ps->SetupChannel((PS_CHANNEL)mOutputChannel, (PS_COUPLING)mOutputChannelCoupling, currentOutputChannelRange, (float)mOutputDcOffset)) )
    {
        return false;
    }

    // If either channel is AC coupling, turn on a delay for settling out DC offsets caused by
    // discontinuities from switching the signal generator.
    delayForAcCoupling = (mInputChannelCoupling == PS_DC && mOutputChannelCoupling == PS_DC) ? false : true;

    // Explicitly turn off the other channels if they exist
    if (numChannels > 2)
    {
        for (int chan = 0; chan < numChannels; chan++)
        {
            if (chan != mInputChannel && chan != mOutputChannel)
            {
                if( !(ps->DisableChannel((PS_CHANNEL)chan) ))
                {
                    return false;
                }
            }
        }
    }

    // Get the maximum scope samples per channel
    if (!(ps->GetMaxSamples(&maxScopeSamplesPerChannel)))
    {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::ExecuteFRA
//
// Purpose: Carries out the execution of the FRA
//
// Parameters: [in] startFreqHz - Beginning frequency
//             [in] stopFreqHz - End frequency
//             [in] stepsPerDecade - Steps per decade
//             [out] return - Whether the function was successful.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::ExecuteFRA(double startFreqHz, double stopFreqHz, int stepsPerDecade )
{
    mStartFreqHz = startFreqHz;
    mStopFreqHz = stopFreqHz;
    mStepsPerDecade = stepsPerDecade;

    DWORD dwWaitResult;
    DWORD winError;

    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];

    GenerateFrequencyPoints();
    AllocateFraData();

    cancel = false;
    if (TRUE != (ResetEvent( hCaptureEvent )))
    {
        winError = GetLastError();
        wsprintf( fraStatusText, L"Fatal error: Failed to reset capture event: %d", winError );
        UpdateStatus( fraStatusMsg, FRA_STATUS_FATAL_ERROR, fraStatusText );
        return false;
    }

    // Update the status to indicate the FRA has started
    UpdateStatus( fraStatusMsg, FRA_STATUS_PROGRESS, 0, numSteps );

    for (freqStepCounter = 0; freqStepCounter < numSteps; freqStepCounter++)
    {
        currentFreqHz = freqsHz[freqStepCounter];
        for (autorangeRetryCounter = 0; autorangeRetryCounter < maxAutorangeRetries; autorangeRetryCounter++)
        {
            wsprintf( fraStatusText, L"Status: Starting frequency step %d, range try %d", freqStepCounter+1, autorangeRetryCounter+1 );
            UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
            if (true != StartCapture( currentFreqHz ))
            {
                return false;
            }
            // Adjust the delay time for a safety factor of 1.5x and never let it go less than 3 seconds
            timeIndisposedMs = max( 3000, (timeIndisposedMs*3)/2);
            dwWaitResult = WaitForSingleObject( hCaptureEvent, timeIndisposedMs );

            if (cancel)
            {
                // Notify of cancellation
                UpdateStatus( fraStatusMsg, FRA_STATUS_CANCELED, freqStepCounter, numSteps );
                return false;
            }

            if (dwWaitResult == WAIT_OBJECT_0)
            {
                if (true != GetData())
                {
                    return false;
                }

                if (false == CheckSignalOverflows())
                {
                    // Both channels are over-range, don't bother with further analysis.
                    continue; // Try again on a different range
                }
                if (false == CheckSignalRanges())
                {
                    // At least one of the channels needs adjustment
                    continue; // Try again on a different range
                }
                else // Data is good, calculate and move on to next frequency
                {
                    // Currently no error is possible so just cast to void
                    (void)CalculateGainAndPhase( &gainsDb[freqStepCounter], &phasesDeg[freqStepCounter] );
                    autorangeRetryCounter++; // reflect the attempt that succeeded

                    // Notify progress
                    UpdateStatus( fraStatusMsg, FRA_STATUS_PROGRESS, freqStepCounter+1, numSteps );
                    break;
                }
            }
            else
            {
                UpdateStatus( fraStatusMsg, FRA_STATUS_FATAL_ERROR, L"Fatal Error: Data capture wait timed out" );
                return false;
            }
        }

        // Make records for diagnostics
        sampleInterval[freqStepCounter] = 1.0 / actualSampFreqHz;
        diagNumSamples[freqStepCounter] = inputMinData[freqStepCounter][0].size();
        autoRangeTries[freqStepCounter] = autorangeRetryCounter;

        if (autorangeRetryCounter == maxAutorangeRetries)
        {
            // This is a temporary solution until we implement a fully interactive one.
            UpdateStatus( fraStatusMsg, FRA_STATUS_AUTORANGE_LIMIT, inputChannelAutorangeStatus, outputChannelAutorangeStatus );
            if (true == fraStatusMsg.responseData.proceed)
            {
                gainsDb[freqStepCounter] = 0.0;
                phasesDeg[freqStepCounter] = 0.0;
                // Notify progress
                UpdateStatus( fraStatusMsg, FRA_STATUS_PROGRESS, freqStepCounter+1, numSteps );
            }
            else
            {
                // Notify of cancellation
                UpdateStatus( fraStatusMsg, FRA_STATUS_CANCELED, freqStepCounter, numSteps );
                return false;
            }
        }
    }

    TransferLatestResults();

    if (mDiagnosticsOn)
    {
        GenerateDiagnosticOutput();
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::CancelFRA
//
// Purpose: Cancels the FRA execution
//
// Parameters: [out] return - Whether the function was successful.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::CancelFRA()
{
    cancel = true;
    SetEvent( hCaptureEvent ); // To break it out of waiting to capture data in case
    // there are several seconds of data to capture
    return true; // bool return reserved for possible future event based signalling that may fail
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::CheckSignalOverflows
//
// Purpose: Makes auto-ranging decisions based on channel over-voltage info.
//
// Parameters: [out] return: whether to proceed with further computation, based on whether both
//                           channels are over-voltage
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::CheckSignalOverflows(void)
{
    bool retVal = true;

    // Reset to default
    inputChannelAutorangeStatus = OK;
    outputChannelAutorangeStatus = OK;

    // Make records for diagnostics
    inOV[freqStepCounter][autorangeRetryCounter] = ovIn;
    outOV[freqStepCounter][autorangeRetryCounter] = ovOut;
    inRange[freqStepCounter][autorangeRetryCounter] = currentInputChannelRange;
    outRange[freqStepCounter][autorangeRetryCounter] = currentOutputChannelRange;

    if (ovIn)
    {
        if (currentInputChannelRange < maxRange)
        {
            inputChannelAutorangeStatus = CHANNEL_OVERFLOW;
            currentInputChannelRange = (PS_RANGE)((int)currentInputChannelRange + 1);
        }
        else
        {
            inputChannelAutorangeStatus = HIGHEST_RANGE_LIMIT_REACHED;
        }
    }
    if (ovOut)
    {
        if (currentOutputChannelRange < maxRange)
        {
            outputChannelAutorangeStatus = CHANNEL_OVERFLOW;
            currentOutputChannelRange = (PS_RANGE)((int)currentOutputChannelRange + 1);
        }
        else
        {
            outputChannelAutorangeStatus = HIGHEST_RANGE_LIMIT_REACHED;
        }
    }

    if (ovIn && ovOut) // If both are over-range, signal to skip further processing.
    {
        retVal = false;
        // Initialize the diagnostic record since CheckSignalRanges will not run
        inAmps[freqStepCounter][autorangeRetryCounter] = 0.0;
        outAmps[freqStepCounter][autorangeRetryCounter] = 0.0;
    }

    // Clear overflow
    ovIn = ovOut = false;

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::CheckSignalRanges
//
// Purpose: Makes auto-ranging decisions based on signal max amplitude info.
//
// Parameters: [out] return: whether to proceed with further computation, based on whether both
//                           in-range
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::CheckSignalRanges(void)
{

    bool retVal = true;
    double inputAmplitude = 0.0;
    double outputAmplitude = 0.0;

    // Want amplitude to be above a lower threshold to avoid excessive quantization noise.
    // Want the signal to be below an upper threshold to avoid being close to overflow.

    // Check Input
    if (inputChannelAutorangeStatus == OK)
    {
        Goertzel( inputBuffer.data(), numSamples, actualSampFreqHz, currentFreqHz,
                  currentInputMagnitude, currentInputPhase, inputAmplitude, currentInputPurity );

        if (((double)inputAbsMax[freqStepCounter][autorangeRetryCounter]/rangeCounts) > maxAmplitudeRatio)
        {
            if (currentInputChannelRange < maxRange)
            {
                currentInputChannelRange = (PS_RANGE)((int)currentInputChannelRange + 1);
                inputChannelAutorangeStatus = AMPLITUDE_TOO_HIGH;
            }
            else
            {
                inputChannelAutorangeStatus = HIGHEST_RANGE_LIMIT_REACHED;
            }
            retVal = false;
        }
        else if (((double)inputAbsMax[freqStepCounter][autorangeRetryCounter]/rangeCounts) <
                 (maxAmplitudeRatio/rangeInfo[currentInputChannelRange].ratioDown - minAmplitudeRatioTolerance))
        {
            if (currentInputChannelRange > minRange)
            {
                currentInputChannelRange = (PS_RANGE)((int)currentInputChannelRange - 1);
                inputChannelAutorangeStatus = AMPLITUDE_TOO_LOW;
                retVal = false;
            }
            else
            {
                if (((double)inputAbsMax[freqStepCounter][autorangeRetryCounter]/rangeCounts) < minAllowedAmplitudeRatio)
                {
                    inputChannelAutorangeStatus = LOWEST_RANGE_LIMIT_REACHED;
                    retVal = false;
                }
                else
                {
                    // Do nothing
                }
            }
        }
        else
        {
            // Do nothing
        }
    }
    else
    {
        retVal = false; // Still need to adjust for the overflow on this channel
        inputAmplitude = 0.0;
    }

    // Check Output
    if (outputChannelAutorangeStatus == OK)
    {
        Goertzel( outputBuffer.data(), numSamples, actualSampFreqHz, currentFreqHz,
                  currentOutputMagnitude, currentOutputPhase, outputAmplitude, currentOutputPurity );

        if (((double)outputAbsMax[freqStepCounter][autorangeRetryCounter]/rangeCounts) > maxAmplitudeRatio)
        {
            if (currentOutputChannelRange < maxRange)
            {
                currentOutputChannelRange = (PS_RANGE)((int)currentOutputChannelRange + 1);
                outputChannelAutorangeStatus = AMPLITUDE_TOO_HIGH;
            }
            else
            {
                outputChannelAutorangeStatus = HIGHEST_RANGE_LIMIT_REACHED;
            }
            retVal = false;
        }
        else if (((double)outputAbsMax[freqStepCounter][autorangeRetryCounter]/rangeCounts) <
                 (maxAmplitudeRatio/rangeInfo[currentOutputChannelRange].ratioDown - minAmplitudeRatioTolerance))
        {
            if (currentOutputChannelRange > minRange)
            {
                currentOutputChannelRange = (PS_RANGE)((int)currentOutputChannelRange - 1);
                outputChannelAutorangeStatus = AMPLITUDE_TOO_LOW;
                retVal = false;
            }
            else
            {
                if (((double)outputAbsMax[freqStepCounter][autorangeRetryCounter]/rangeCounts) < minAllowedAmplitudeRatio)
                {
                    outputChannelAutorangeStatus = LOWEST_RANGE_LIMIT_REACHED;
                    retVal = false;
                }
                else
                {
                    // Do nothing
                }
            }
        }
        else
        {
            // Do nothing
        }
    }
    else
    {
        retVal = false; // Still need to adjust for the overflow on this channel
        outputAmplitude = 0.0;
    }

    // Make records for diagnostics
    inAmps[freqStepCounter][autorangeRetryCounter] = inputAmplitude;
    outAmps[freqStepCounter][autorangeRetryCounter] = outputAmplitude;
    inputPurity[freqStepCounter][autorangeRetryCounter] = currentInputPurity;
    outputPurity[freqStepCounter][autorangeRetryCounter] = currentOutputPurity;

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::GetResults
//
// Purpose: To get the results from the the most recently executed Frequency Response Analysis
//
// Parameters:
//    [out] numSteps - the number of frequency steps taken (also the size of the other arrays}
//    [out] freqsLogHz - array of frequency points taken, expressed in log base 10 Hz
//    [out] gainsDb - array of gains at each frequency point of freqsLogHz, expressed in dB
//    [out] phasesDeg - array of phase shifts at each frequency point of freqsLogHz, expressed in degrees
//
// Notes: The memory returned in the pointers is only valid until the next FRA execution or
//        destruction of the PicoScope FRA object.  If there is no valid data, numSteps
//        is set to 0.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::GetResults( int* numSteps, double** freqsLogHz, double** gainsDb, double** phasesDeg )
{
    *numSteps = latestCompletedNumSteps;
    *freqsLogHz = latestCompletedFreqsLogHz.data();
    *gainsDb = latestCompletedGainsDb.data();
    *phasesDeg = latestCompletedPhasesDeg.data();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::TransferLatestResults
//
// Purpose: Transfers the latest results into storage.  Used at the completion of the FRA to
//          facilitate not writing over the old results until we have to.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::TransferLatestResults(void)
{
    latestCompletedNumSteps = numSteps;
    latestCompletedFreqsLogHz = freqsLogHz;
    latestCompletedGainsDb = gainsDb;
    latestCompletedPhasesDeg = phasesDeg;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::GenerateFrequencyPoints
//
// Purpose: Given the user's selection of start frequency, stop frequency, and steps per decade,
//          compute the frequency step points.
//
// Parameters: N/A
//
// Notes: Because the scopes have output frequency precision limits, this function also 
//        "patches up" the results to align to freuencies the scope is capable of
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::GenerateFrequencyPoints(void)
{
    double logStartFreqHz, logStopFreqHz;

    logStartFreqHz = log10(mStartFreqHz);
    logStopFreqHz = log10(mStopFreqHz);
    double logStepSize;

    // ceil to account for the fractional leftover and thus allow reaching the stop frequency
    // +1 to account for the start frequency
    numSteps = (int)(ceil((logStopFreqHz - logStartFreqHz)*mStepsPerDecade)) + 1;
    logStepSize = 1.0 / mStepsPerDecade;

    freqsHz.resize(numSteps);
    freqsLogHz.resize(numSteps);
    gainsDb.resize(numSteps);
    phasesDeg.resize(numSteps);

    // Loop up to the second-to-last frequency point and
    // fill in the last one as the end frequency
    for (int i = 0; i < numSteps-1; i++)
    {
        freqsLogHz[i] = logStartFreqHz + i*logStepSize;
        freqsHz[i] = pow( 10.0, freqsLogHz[i] );
    }
    freqsLogHz[numSteps-1] = logStopFreqHz;
    freqsHz[numSteps-1] = mStopFreqHz;

    // Now "patch-up" the frequencies to account for the precision
    // limitations of the signal generator
    for (int i = 0; i < numSteps; i++)
    {
        uint32_t N = (uint32_t)(freqsHz[i] / signalGeneratorPrecision);
        freqsHz[i] = N * signalGeneratorPrecision;
        freqsLogHz[i] = log10( freqsHz[i] );
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::AllocateFraData
//
// Purpose: Allocates data that the FRA will use to store input data, intermediate results, and 
//          diagnotic data.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::AllocateFraData(void)
{
    int i;

    inputBuffer.resize(maxScopeSamplesPerChannel);
    outputBuffer.resize(maxScopeSamplesPerChannel);

    inAmps.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inAmps[i].resize(maxAutorangeRetries);
    }

    outAmps.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outAmps[i].resize(maxAutorangeRetries);
    }

    inOV.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inOV[i].resize(maxAutorangeRetries);
    }

    outOV.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outOV[i].resize(maxAutorangeRetries);
    }

    inRange.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inRange[i].resize(maxAutorangeRetries);
    }

    outRange.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outRange[i].resize(maxAutorangeRetries);
    }

    inputMinData.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inputMinData[i].resize(maxAutorangeRetries);
    }

    outputMinData.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outputMinData[i].resize(maxAutorangeRetries);
    }

    inputMaxData.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inputMaxData[i].resize(maxAutorangeRetries);
    }

    outputMaxData.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outputMaxData[i].resize(maxAutorangeRetries);
    }

    inputAbsMax.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inputAbsMax[i].resize(maxAutorangeRetries);
    }

    outputAbsMax.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outputAbsMax[i].resize(maxAutorangeRetries);
    }

    inputPurity.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inputPurity[i].resize(maxAutorangeRetries);
    }

    outputPurity.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outputPurity[i].resize(maxAutorangeRetries);
    }

    diagNumSamples.resize(numSteps);
    autoRangeTries.resize(numSteps);
    sampleInterval.resize(numSteps);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::GenerateDiagnosticOutput
//
// Purpose: Creates time-domain plots of data gathered to support diagnosis of any problems 
//          encountered during the FRA - e.g. outputs that seem incorrect.
//
// Parameters: N/A
//
// Notes: Plots are stored in the user's Documents/FRA4PicoScope/diag directory
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::GenerateDiagnosticOutput(void)
{
    stringstream fileName;
    stringstream inputTitle;
    stringstream outputTitle;
    int maxSamples;
    vector<double> times;
    vector<double> inputMinVoltages;
    vector<double> outputMinVoltages;
    vector<double> inputMaxVoltages;
    vector<double> outputMaxVoltages;
    double inputAmplitude, outputAmplitude;

    maxSamples = *max_element(begin(diagNumSamples),end(diagNumSamples));
    times.resize(maxSamples);
    inputMinVoltages.resize(maxSamples);
    outputMinVoltages.resize(maxSamples);
    if (mSamplingMode == HIGH_NOISE)
    {
        inputMaxVoltages.resize(maxSamples);
        outputMaxVoltages.resize(maxSamples);
    }

    for( int il = 0; il < numSteps; il++)
    {
        for (int jl = 0; jl < autoRangeTries[il]; jl++ )
        {
            fileName.clear();
            fileName.str("");
            inputTitle.clear();
            inputTitle.str("");
            outputTitle.clear();
            outputTitle.str("");

            plsdev( "svg" );
            fileName << mBaseDataPath << "diag\\step" << il << "try" << jl << ".svg";
            plsfnam( fileName.str().c_str() );

            plstar( 1, 2 ); // Setup to stack the input and output plots

            // Plot the data.
            for( int kl = 0; kl < diagNumSamples[il]; kl++)
            {
                times[kl] = ((double)kl)*sampleInterval[il];
            }
            for( int kl = 0; kl < diagNumSamples[il]; kl++)
            {
                inputMinVoltages[kl] = ((double)inputMinData[il][jl][kl] / 32768.0) * rangeInfo[inRange[il][jl]].rangeVolts;
            }
            for( int kl = 0; kl < diagNumSamples[il]; kl++)
            {
                outputMinVoltages[kl] = ((double)outputMinData[il][jl][kl] / 32768.0) * rangeInfo[outRange[il][jl]].rangeVolts;
            }
            if (mSamplingMode == HIGH_NOISE)
            {
                for( int kl = 0; kl < diagNumSamples[il]; kl++)
                {
                    inputMaxVoltages[kl] = ((double)inputMaxData[il][jl][kl] / 32768.0) * rangeInfo[inRange[il][jl]].rangeVolts;
                }
                for( int kl = 0; kl < diagNumSamples[il]; kl++)
                {
                    outputMaxVoltages[kl] = ((double)outputMaxData[il][jl][kl] / 32768.0) * rangeInfo[outRange[il][jl]].rangeVolts;
                }
            }

            // Plot input
            plenv( 0.0, diagNumSamples[il]*sampleInterval[il],
                   -rangeInfo[inRange[il][jl]].rangeVolts, rangeInfo[inRange[il][jl]].rangeVolts,
                   0, 0 );
            plcol0(1);
            if (mSamplingMode == LOW_NOISE)
            {
                plline( diagNumSamples[il], times.data(), inputMinVoltages.data() );
            }
            else
            {
                for (int kl = 0; kl < diagNumSamples[il]; kl++)
                {
                    pljoin( times[kl], inputMinVoltages[kl], times[kl], inputMaxVoltages[kl] );
                }
            }

            // Draw input amplitude lines if no overflow
            plcol0(2);
            if (!inOV[il][jl])
            {
                inputAmplitude = (inAmps[il][jl] / 32768.0) * rangeInfo[inRange[il][jl]].rangeVolts;
                pljoin( 0.0, inputAmplitude, diagNumSamples[il]*sampleInterval[il], inputAmplitude);
                pljoin( 0.0, -inputAmplitude, diagNumSamples[il]*sampleInterval[il], -inputAmplitude);
                inputTitle << "Input signal; Amplitude: " << setprecision(6) << inputAmplitude << " V;" << "Purity: " << setprecision(6) << inputPurity[il][jl];
            }
            else
            {
                inputTitle << "Input signal; Overrange";
            }

            plcol0(1);
            pllab( "Time (s)", "Volts", inputTitle.str().c_str() );

            // Plot output
            plenv( 0.0, diagNumSamples[il]*sampleInterval[il],
                   -rangeInfo[outRange[il][jl]].rangeVolts, rangeInfo[outRange[il][jl]].rangeVolts,
                   0, 0 );
            plcol0(1);

            if (mSamplingMode == LOW_NOISE)
            {
                plline( diagNumSamples[il], times.data(), outputMinVoltages.data() );
            }
            else
            {
                for (int kl = 0; kl < diagNumSamples[il]; kl++)
                {
                    pljoin( times[kl], outputMinVoltages[kl], times[kl], outputMaxVoltages[kl] );
                }
            }

            // Draw output amplitude lines if no overflow
            plcol0(2);
            if (!outOV[il][jl])
            {
                outputAmplitude = (outAmps[il][jl] / 32768.0) * rangeInfo[outRange[il][jl]].rangeVolts;
                pljoin( 0.0, outputAmplitude, diagNumSamples[il]*sampleInterval[il], outputAmplitude);
                pljoin( 0.0, -outputAmplitude, diagNumSamples[il]*sampleInterval[il], -outputAmplitude);
                outputTitle << "Output signal; Amplitude: " << setprecision(6) << outputAmplitude << " V;" << "Purity: " << setprecision(6) << outputPurity[il][jl];
            }
            else
            {
                outputTitle << "Output signal; Overrange";
            }

            plcol0(1);
            pllab( "Time (s)", "Volts", outputTitle.str().c_str() );

            // Close PLplot library
            plend();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::StartCapture
//
// Purpose: Sets up the signal generator, and starts a data capture.
//
// Parameters: [in] measFreqHz - The frequency which we are currently measuring.
//             [out] return - Whether the function was successful.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::StartCapture( double measFreqHz )
{
    uint32_t timebase;
    uint32_t numCycles;

    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];

    // Only setup the signal generator on the first auto-range try
    if (autorangeRetryCounter == 0)
    {
        swprintf( fraStatusText, 128, L"Status: Setting input frequency to %0.3lf Hz", measFreqHz );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );

        if (!(ps->SetSignalGenerator((float)currentOutputVolts, (float)measFreqHz)))
        {
            return false;
        }

        if (mExtraSettlingTimeMs > 0)
        {
            Sleep( mExtraSettlingTimeMs );
        }
    }

    wsprintf( fraStatusText, L"Status: Setting input channel range to %s", rangeInfo[currentInputChannelRange].name );
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
    if( !(ps->SetupChannel((PS_CHANNEL)mInputChannel, (PS_COUPLING)mInputChannelCoupling, currentInputChannelRange, (float)mInputDcOffset)) )
    {
        return false;
    }

    wsprintf( fraStatusText, L"Status: Setting output channel range to %s", rangeInfo[currentOutputChannelRange].name );
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
    if( !(ps->SetupChannel((PS_CHANNEL)mOutputChannel, (PS_COUPLING)mOutputChannelCoupling, currentOutputChannelRange, (float)mOutputDcOffset)) )
    {
        return false;
    }

    // Set no triggers
    if (!(ps->DisableChannelTriggers()))
    {
        return false;
    }

    if (mSamplingMode == LOW_NOISE)
    {
        // Setup the sampling frequency and number of samples.  Criteria:
        // - In order for the amplitude calculation to be accurate, and minimize
        //   spectral leakage, we need to try to capture an integer number of 
        //   periods of the signal of interest.
        // - Sampling frequency well above Nyquist (at least 64x)
        // - Enough samples that the selection bin is narrow.
        if (!(ps->GetTimebase(measFreqHz*64.0, &actualSampFreqHz, &timebase)))
        {
            return false;
        }
        // Calculate actual sample frequency, and number of samples, collecting enough
        // samples for 16 cycles of the measured frequency.
        numCycles = 16;
        double samplesPerCycle = actualSampFreqHz / measFreqHz;
        // Deferring the integer truncation till this point ensures
        // the least inaccuracy in hitting the integer periods criteria
        numSamples = (int32_t)(samplesPerCycle * (double)numCycles) + 1;
    }
    else
    {
        // Goertzel bin width divided by selection frequency is the inverse of the number of periods
        // sampled.  So, attempt to set bin +/- 0.5% around the selection frequency => 100 periods.
        // This is easy to achieve at higher frequencies, but at lower frequencies, we may not have the
        // buffer space.
        numCycles = min( 100, (uint32_t)((measFreqHz * (double)maxScopeSamplesPerChannel) / fSampNoiseRejectMode) );
        numSamples = (int32_t)(((double)numCycles * fSampNoiseRejectMode) / measFreqHz) + 1;

        timebase = timebaseNoiseRejectMode;
        actualSampFreqHz = fSampNoiseRejectMode;
    }

    // Insert a progressive delay to settle out DC offsets caused by
    // discontinuities from switching the signal generator.
    if (delayForAcCoupling)
    {
        Sleep( 200*autorangeRetryCounter );
    }

    // Setup block mode
    if (!(ps->RunBlock(numSamples, timebase, &timeIndisposedMs, DataReady, &hCaptureEvent)))
    {
        return false;
    }

    swprintf( fraStatusText, 128, L"Status: Capturing %d samples (%d cycles) at %.3lg Hz takes %0.1lf sec.", numSamples, numCycles, actualSampFreqHz, (double)timeIndisposedMs/1000.0 );
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::GetData
//
// Purpose: Retrieve the data the scope has captured.
//
// Parameters: [out] return - Whether the function was successful.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::GetData(void)
{
    bool retVal = true;
    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];
    uint32_t compressedBufferSize;

    wsprintf( fraStatusText, L"Status: Transferring %d samples", numSamples );
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );

    compressedBufferSize = (mSamplingMode == LOW_NOISE) ? 0 : 1024;

    if (!(ps->GetData(numSamples, compressedBufferSize, mInputChannel, mOutputChannel, inputBuffer, outputBuffer,
                      inputMinData[freqStepCounter][autorangeRetryCounter], outputMinData[freqStepCounter][autorangeRetryCounter],
                      inputMaxData[freqStepCounter][autorangeRetryCounter], outputMaxData[freqStepCounter][autorangeRetryCounter],
                      inputAbsMax[freqStepCounter][autorangeRetryCounter], outputAbsMax[freqStepCounter][autorangeRetryCounter],
                      &ovIn, &ovOut)))
    {
        retVal = false;
    }

    return retVal;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::CalculateGainAndPhase
//
// Purpose: Calculate gain and phase shift of the input and output signals.
//
// Parameters: [out] gain - gain of input over output
//             [out] phase = phase shift from input to output
//             [out] return - Whether the function was successful.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////
 
bool PicoScopeFRA::CalculateGainAndPhase( double* gain, double* phase)
{
    double tempPhase;

    // Compute gain as dB
    // Compute channel range gain factor
    double channelGainFactor = rangeInfo[currentOutputChannelRange].rangeVolts / rangeInfo[currentInputChannelRange].rangeVolts;
    *gain = 20.0 * log10( channelGainFactor * currentOutputMagnitude / currentInputMagnitude );

    // Compute phase in degrees, limiting to +/-180
    tempPhase = currentOutputPhase - currentInputPhase;
    if (tempPhase > M_PI)
    {
        tempPhase -= 2*M_PI;
    }
    else if (tempPhase < -M_PI)
    {
        tempPhase += 2*M_PI;
    }

    *phase = tempPhase*180.0/M_PI;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::Goertzel
//
// Purpose: The Goertzel algorithm is a fast method of computing a single point DFT.  This is a generalized Goertzel
// algorithm which allows for a non-integer bin (and thus is really a single point DTFT).  Both magnitude and phase
// are returned.  The advantage of performing the Goertzel algorithm vs an FFT is that for our application we're only
// interested in a single frequency per measurement, and thus Goertzel is faster than FFT.  The advantage of the generalized
// Goertzel (k ∈ R) is that we don't have to adjust the number of samples or sampling rate to maintain accuracy of the
// frequency selection.  This function also computes other useful parameters such as amplitude and purity, which can be used 
// for data quality decisions.
//
// Parameters: [in] samples - data sample points
//             [in] N - number of data points
//             [in] fSamp - frequency of the sampling
//             [in] fDetect - frequency to detect  
//             [output] magnitude - The Goertzel magnitude
//             [output] phase - The Goertzel phase
//             [output] amplitude - The measured amplitude of the signal
//             [output] purity - The purity of the signal (signal power over total power)
//
// Notes: 
//
// The following reference:
//    - derives the generalized Gortzel
//    - is the source of the algorithm used in this method
//			doi:10.1186/1687-6180-2012-56
//			Sysel and Rajmic:
//			Goertzel algorithm generalized to non-integer multiples of fundamental frequency.
//			EURASIP Journal on Advances in Signal Processing 2012 2012:56.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::Goertzel( int16_t* samples, uint32_t N, double fSamp, double fDetect, double& magnitude, double& phase, double& amplitude, double& purity )
{
    double A, B;
    complex<double> C, D, y;
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    double totalPower = 0.0;
    double signalPower = 0.0;
    long long sum = 0;
    double average = 0.0;
    uint32_t i;

    // Calculate average to subtract out DC, because we will not count DC as noise
    for (i = 0; i < N; i++)
    {
        sum+=samples[i];
    }
    average = (double)sum/(double)N;

    A = 2.0 * M_PI * fDetect / fSamp; // A condensed version of: k = N * fDetect / fSamp; A = 2.0 * M_PI * k / N
    B = 2.0 * cos( A );
    C = exp( complex<double>(0.0,-A) );
    D = exp( complex<double>(0.0,-A*(N-1)) );

    for (i = 0; i < N-1; i++)
    {
        totalPower += ((samples[i]-average)*(samples[i]-average));

        s0 = (samples[i]-average) + B * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    totalPower += ((samples[i]-average)*(samples[i]-average));

    s0 = (samples[i]-average) + B * s1 - s2;

    y = s0 - s1 * C;

    y = y * D;

    magnitude = abs( y );
    phase = arg( y );

    // Using N+1 because this form of the Goertzel iterates N+1 times, with x[N] = 0, thus effectively using N+1 samples.
    // The x[N]=0 sample has no effect on the time domain Parseval's energy calculation.
    amplitude = 2.0 * magnitude / (N+1);
    signalPower = 2.0 * (magnitude * magnitude) / (N+1);

    purity = signalPower/totalPower;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::~PicoScopeFRA
//
// Purpose: Destructor
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

PicoScopeFRA::~PicoScopeFRA(void)
{
    if (NULL != hCaptureEvent)
    {
        (void)CloseHandle(hCaptureEvent);
    }
}