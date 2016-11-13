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
#include <boost/math/special_functions/round.hpp>
#include <complex>
#include <algorithm>
#include <memory>
#include <vector>
#include <intrin.h>
#include <sstream>
#include <iomanip>
#include "plplot.h" // For creating diagnostic graphs

#define WORKAROUND_PS_TIMEINDISPOSED_BUG

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
    PicoScopeFRA::SetCaptureStatus( status );
    SetEvent( *(HANDLE*)pParameter );
}

const double PicoScopeFRA::attenInfo[] = {1.0, 10.0, 20.0, 100.0, 200.0, 1000.0};
const double PicoScopeFRA::inputRangeInitialEstimateMargin = 0.95;
const uint32_t PicoScopeFRA::timeDomainDiagnosticDataLengthLimit = 1024;

PICO_STATUS PicoScopeFRA::captureStatus;

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
    autoStimulusInputChannelRange = (PS_RANGE)0;
    autoStimulusOutputChannelRange = (PS_RANGE)0;

    mSamplingMode = LOW_NOISE;

    numSteps = latestCompletedNumSteps = freqStepCounter = freqStepIndex = 0;

    pInputBuffer = pOutputBuffer = NULL;

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
    mSweepDescending = false;
    mAdaptiveStimulus = false;
    mTargetResponseAmplitude = 0.0;
    mTargetResponseAmplitudeTolerance = 0.0;
    maxAdaptiveStimulusRetries = 0;
    mPhaseWrappingThreshold = 180.0;
    rangeCounts = 0.0;
    signalGeneratorPrecision = 0.0;
    autorangeRetryCounter = 0;
    adaptiveStimulusRetryCounter = 0;
    stimulusChanged = false;
    mDiagnosticsOn = false;
    rangeInfo = NULL;
    inputMinRange = 0;
    inputMaxRange = 0;
    outputMinRange = 0;
    outputMaxRange = 0;
    captureStatus = PICO_OK;
    ps = NULL;
    numAvailableChannels = 2;
    maxScopeSamplesPerChannel = 0;
    currentFreqHz = 0.0;
    currentStimulusVpp = 0.0;
    mMaxStimulusVpp = 0.0;
    currentOutputAmplitudeVolts = 0.0;
    currentInputAmplitudeVolts = 0.0;
    mStartFreqHz = 0.0;
    mStopFreqHz = 0.0;
    mStepsPerDecade = 10;
    mInputChannel = PS_CHANNEL_A;
    mOutputChannel = PS_CHANNEL_B;
    mInputChannelCoupling = PS_AC;
    mOutputChannelCoupling = PS_AC;
    mInputChannelAttenuation = ATTEN_1X;
    mOutputChannelAttenuation = ATTEN_1X;

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
    numAvailableChannels = ps->GetNumChannels();
    rangeInfo = ps->GetRangeCaps();
    signalGeneratorPrecision = ps->GetSignalGeneratorPrecision();
    rangeCounts = ps->GetMaxValue();

    // Setup arbitrary initial settings to force a calculation of maxScopeSamplesPerChannel
    SetupChannels( PS_CHANNEL_A, PS_AC, ATTEN_1X, 0.0, PS_CHANNEL_B, PS_AC, ATTEN_1X, 0.0, 0.0, 0.0 );

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
//                                                   take action
//             [in] maxAutorangeAmplitude - Amplitude before we switch to next higher range.
//             [in] minCyclesCaptured - Minimum cycles captured for stmulus signal
//             [in] sweepDescending - if true, sweep from highest frequency to lowest
//             [in] phaseWrappingThreshold - phase value to use as wrapping point (in degrees)
//                                           absolute value should be less than 360
//             [in] diagnosticsOn - Whether to output plots of time domain data
//             [in] baseDataPath - Path providing location to store time domain plots
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::SetFraSettings( SamplingMode_T samplingMode, bool adaptiveStimulusMode, double targetSignalAmplitude,
                                   bool sweepDescending, double phaseWrappingThreshold )
{
    mSamplingMode = samplingMode;
    mAdaptiveStimulus = adaptiveStimulusMode;
    mTargetResponseAmplitude = targetSignalAmplitude;
    mSweepDescending = sweepDescending;
    mPhaseWrappingThreshold = phaseWrappingThreshold;
}

void PicoScopeFRA::SetFraTuning( double purityLowerLimit, uint16_t extraSettlingTimeMs, uint8_t autorangeTriesPerStep,
                                 double autorangeTolerance, double smallSignalResolutionTolerance, double maxAutorangeAmplitude,
                                 uint8_t adaptiveStimulusTriesPerStep, double targetSignalAmplitudeTolerance, uint16_t minCyclesCaptured )
{
    mPurityLowerLimit = purityLowerLimit;
    mExtraSettlingTimeMs = extraSettlingTimeMs;
    maxAutorangeRetries = autorangeTriesPerStep;
    minAmplitudeRatioTolerance = autorangeTolerance;
    minAllowedAmplitudeRatio = smallSignalResolutionTolerance;
    maxAmplitudeRatio = maxAutorangeAmplitude;
    maxAdaptiveStimulusRetries = adaptiveStimulusTriesPerStep;
    mTargetResponseAmplitudeTolerance = targetSignalAmplitudeTolerance;
    mMinCyclesCaptured = minCyclesCaptured;
}

void PicoScopeFRA::EnableDiagnostics(wstring baseDataPath)
{
    mDiagnosticsOn = true;
    mBaseDataPath = baseDataPath;
}

void PicoScopeFRA::DisableDiagnostics(void)
{
    mDiagnosticsOn = false;
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
        return (((ps->GetSignalGeneratorPrecision())/2.0) + ((double)mMinCyclesCaptured * ( ps->GetNoiseRejectModeSampleRate() / (double)maxScopeSamplesPerChannel )));
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
//             [in] initialStimulusVpp - Volts peak to peak of the stimulus signal; initial for
//                                       adaptive stimulus mode, constant otherwise
//             [in] maxStimulusVpp - Maximum volts peak to peak of the stimulus signal; used
//                                   in adaptive stimulus mode.
//             [out] return - Whether the function was successful.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::SetupChannels( int inputChannel, int inputChannelCoupling, int inputChannelAttenuation, double inputDcOffset,
                                  int outputChannel, int outputChannelCoupling, int outputChannelAttenuation, double outputDcOffset,
                                  double initialStimulusVpp, double maxStimulusVpp )
{
    FRA_STATUS_MESSAGE_T fraStatusMsg;
    PS_RANGE inputRange;

    mInputChannelCoupling = (PS_COUPLING)inputChannelCoupling;
    mOutputChannelCoupling = (PS_COUPLING)outputChannelCoupling;

    inputMinRange = ps->GetMinRange(mInputChannelCoupling);
    inputMaxRange = ps->GetMaxRange(mInputChannelCoupling);
    outputMinRange = ps->GetMinRange(mOutputChannelCoupling);
    outputMaxRange = ps->GetMaxRange(mOutputChannelCoupling);

    for (inputRange = inputMaxRange; inputRange > inputMinRange; inputRange--)
    {
        if (initialStimulusVpp > 2.0*rangeInfo[inputRange].rangeVolts*attenInfo[mInputChannelAttenuation]*inputRangeInitialEstimateMargin)
        {
            inputRange++; // We stepped one too far, so backup
            inputRange = min(inputRange, inputMaxRange); // Just in case, so we can't get an illegal range
            break;
        }
    }

    currentInputChannelRange = inputRange;
    currentOutputChannelRange = outputMinRange;

    mInputChannel = (PS_CHANNEL)inputChannel;
    mOutputChannel = (PS_CHANNEL)outputChannel;

    mInputChannelAttenuation = (ATTEN_T)inputChannelAttenuation;
    mOutputChannelAttenuation = (ATTEN_T)outputChannelAttenuation;

    mInputDcOffset = inputDcOffset;
    mOutputDcOffset = outputDcOffset;

    mMaxStimulusVpp = maxStimulusVpp;
    currentStimulusVpp = initialStimulusVpp;

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
    if (numAvailableChannels > 2)
    {
        for (int chan = 0; chan < numAvailableChannels; chan++)
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
    ps->DisableAllDigitalChannels(); // Ignore failures, which may occur if the scope is not connected to aux DC power

    // Get the maximum scope samples per channel
    if (!(ps->GetMaxSamples(&maxScopeSamplesPerChannel)))
    {
        return false;
    }

    // Tell the PicoScope which channels are input and output
    ps->SetChannelDesignations( mInputChannel, mOutputChannel );

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
// Notes: SetupChannels must be called before this function
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::ExecuteFRA(double startFreqHz, double stopFreqHz, int stepsPerDecade )
{
    bool retVal = true;
    mStartFreqHz = startFreqHz;
    mStopFreqHz = stopFreqHz;
    mStepsPerDecade = stepsPerDecade;

    DWORD dwWaitResult;
    DWORD winError;

    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];

    try
    {
        GenerateFrequencyPoints();
        AllocateFraData();

        cancel = false;
        if (TRUE != (ResetEvent( hCaptureEvent )))
        {
            winError = GetLastError();
            wsprintf( fraStatusText, L"Fatal error: Failed to reset capture event: %d", winError );
            UpdateStatus( fraStatusMsg, FRA_STATUS_FATAL_ERROR, fraStatusText );
            throw FraFault();
        }

        // Update the status to indicate the FRA has started
        UpdateStatus( fraStatusMsg, FRA_STATUS_IN_PROGRESS, 0, numSteps );

        freqStepCounter = 1;
        freqStepIndex = mSweepDescending ? numSteps-1 : 0;
        while ((mSweepDescending && freqStepIndex >= 0) || (!mSweepDescending && freqStepIndex < numSteps))
        {
            currentFreqHz = freqsHz[freqStepIndex];
            for (autorangeRetryCounter = 0, adaptiveStimulusRetryCounter = 0;
                 autorangeRetryCounter < maxAutorangeRetries && adaptiveStimulusRetryCounter < maxAdaptiveStimulusRetries; )
            {
                try
                {
                    wsprintf(fraStatusText, L"Status: Starting frequency step %d, range try %d", freqStepCounter, autorangeRetryCounter + 1);
                    UpdateStatus(fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText);
                    if (true != StartCapture(currentFreqHz))
                    {
                        throw FraFault();
                    }
                    // Adjust the delay time for a safety factor of 1.5x and never let it go less than 3 seconds
                    timeIndisposedMs = max(3000, (timeIndisposedMs * 3) / 2);
                    dwWaitResult = WaitForSingleObject(hCaptureEvent, timeIndisposedMs);

                    if (cancel)
                    {
                        // Notify of cancellation
                        UpdateStatus(fraStatusMsg, FRA_STATUS_CANCELED, freqStepCounter, numSteps);
                        throw FraFault();
                    }

                    if (dwWaitResult == WAIT_OBJECT_0)
                    {
                        if (PICO_OK == PicoScopeFRA::captureStatus)
                        {
                            if (false == ProcessData())
                            {
                                // At least one of the channels needs adjustment
                                continue; // Try again on a different range
                            }
                            else // Data is good, calculate and move on to next frequency
                            {
                                // Currently no error is possible so just cast to void
                                (void)CalculateGainAndPhase(&gainsDb[freqStepIndex], &phasesDeg[freqStepIndex]);

                                // Notify progress
                                UpdateStatus(fraStatusMsg, FRA_STATUS_IN_PROGRESS, freqStepCounter, numSteps);
                                break;
                            }
                        }
                        else if (PICO_POWER_SUPPLY_CONNECTED == PicoScopeFRA::captureStatus ||
                                 PICO_POWER_SUPPLY_NOT_CONNECTED == PicoScopeFRA::captureStatus)
                        {
                            throw PicoScope::PicoPowerChange(PicoScopeFRA::captureStatus);
                        }
                        else
                        {
                            wstringstream wssError;
                            wssError << L"Fatal Error: Data capture error: " << PicoScopeFRA::captureStatus;
                            UpdateStatus(fraStatusMsg, FRA_STATUS_FATAL_ERROR, wssError.str().c_str());
                            throw FraFault();
                        }
                    }
                    else
                    {
                        UpdateStatus(fraStatusMsg, FRA_STATUS_FATAL_ERROR, L"Fatal Error: Data capture wait timed out");
                        throw FraFault();
                    }
                }
                catch (const PicoScope::PicoPowerChange& ex)
                {
                    UpdateStatus( fraStatusMsg, FRA_STATUS_POWER_CHANGED, ex.GetState() == PICO_POWER_SUPPLY_CONNECTED );
                    // Change the power state regardless of whether the user wants to continue FRA execution.
                    ps->ChangePower(ex.GetState());
                    if (true == fraStatusMsg.responseData.proceed)
                    {
                        autorangeRetryCounter = -1;
                        continue;
                    }
                    else
                    {
                        throw FraFault();
                    }
                }
            }

            if (mDiagnosticsOn)
            {
                // Make records for diagnostics
                if (LOW_NOISE == mSamplingMode)
                {
                    sampleInterval[freqStepIndex] = 1.0 / actualSampFreqHz;
                }
                else
                {
                    // The data for plotting is downsampled (aggregated)
                    sampleInterval[freqStepIndex] = ((double)numSamples / (double)timeDomainDiagnosticDataLengthLimit) / actualSampFreqHz;
                }
                diagNumSamplesToPlot[freqStepIndex] = inputMinData[freqStepIndex][0].size();
                diagNumSamplesCaptured[freqStepIndex] = numSamples;
                autoRangeTries[freqStepIndex] = autorangeRetryCounter+1;
            }

            if (autorangeRetryCounter == maxAutorangeRetries ||
                adaptiveStimulusRetryCounter == maxAdaptiveStimulusRetries)
            {
                // This is a temporary solution until we implement a fully interactive one.
                // TODO - change to FRA_STATUS_RETRY_LIMIT_REACHED and parameterize so we can tell user about all ranging issues (measurement and stimulus) in one interaction
                UpdateStatus( fraStatusMsg, FRA_STATUS_AUTORANGE_LIMIT, inputChannelAutorangeStatus, outputChannelAutorangeStatus );
                if (true == fraStatusMsg.responseData.proceed)
                {
                    gainsDb[freqStepIndex] = 0.0;
                    phasesDeg[freqStepIndex] = 0.0;
                    // Notify progress
                    UpdateStatus( fraStatusMsg, FRA_STATUS_IN_PROGRESS, freqStepCounter, numSteps );
                }
                else
                {
                    // Notify of cancellation
                    UpdateStatus( fraStatusMsg, FRA_STATUS_CANCELED, freqStepCounter, numSteps );
                    throw FraFault();
                }
            }

            if (mSweepDescending)
            {
                freqStepIndex--;
            }
            else
            {
                freqStepIndex++;
            }
            freqStepCounter++;
        }

        // Generate any alternate forms
        UnwrapPhases();

        TransferLatestResults();

        UpdateStatus(fraStatusMsg, FRA_STATUS_COMPLETE, freqStepCounter, numSteps);

        if (mDiagnosticsOn)
        {
            // Don't let failure to generate diagnostic plots be fatal.
            try
            {
                GenerateDiagnosticOutput();
            }
            catch (const runtime_error& e)
            {
                wstringstream wssError;
                wssError << e.what();
                UpdateStatus( fraStatusMsg, FRA_STATUS_FATAL_ERROR, wssError.str().c_str() );
            }
        }
    }
    catch (const FraFault& e)
    {
        UNREFERENCED_PARAMETER(e);
        retVal = false;
    }
    catch (const runtime_error& e)
    {
        wstringstream wssError;
        wssError << L"FRA execution error: " << e.what();
        UpdateStatus( fraStatusMsg, FRA_STATUS_FATAL_ERROR, wssError.str().c_str() );
        retVal = false;
    }
    catch (const bad_alloc& e)
    {
        UNREFERENCED_PARAMETER(e);
        wstringstream wssError;
        wssError << L"FRA execution error: Failed to allocate memory.";
        UpdateStatus( fraStatusMsg, FRA_STATUS_FATAL_ERROR, wssError.str().c_str() );
        retVal = false;
    }

    try
    {
        (void)ps->DisableSignalGenerator();
    }
    catch (const exception& e)
    {
        UNREFERENCED_PARAMETER(e);
        retVal = false;
    }

    return retVal;
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
// Name: PicoScopeFRA::SetCaptureStatus
//
// Purpose: Communicate the status value from the capture callback
//
// Parameters: [in] status - the status value from the capture callback
//
// Notes:
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::SetCaptureStatus(PICO_STATUS status)
{
    PicoScopeFRA::captureStatus = status;
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
#if 0
    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];
#endif

    // Reset to default
    inputChannelAutorangeStatus = OK;
    outputChannelAutorangeStatus = OK;

    // Make records for diagnostics
    inOV[freqStepIndex][autorangeRetryCounter] = ovIn;
    outOV[freqStepIndex][autorangeRetryCounter] = ovOut;
    inRange[freqStepIndex][autorangeRetryCounter] = currentInputChannelRange;
    outRange[freqStepIndex][autorangeRetryCounter] = currentOutputChannelRange;

    if (ovIn)
    {
        if (currentInputChannelRange < inputMaxRange)
        {
            inputChannelAutorangeStatus = CHANNEL_OVERFLOW;
            currentInputChannelRange = (PS_RANGE)((int)currentInputChannelRange + 1);
        }
        else
        {
            inputChannelAutorangeStatus = HIGHEST_RANGE_LIMIT_REACHED;
        }
#if 0 // Determine when to log this once we have a log verbosity configuration
        wsprintf( fraStatusText, L"Status: Input signal over-range" );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
#endif
    }
    if (ovOut)
    {
        if (currentOutputChannelRange < outputMaxRange)
        {
            outputChannelAutorangeStatus = CHANNEL_OVERFLOW;
            currentOutputChannelRange = (PS_RANGE)((int)currentOutputChannelRange + 1);
        }
        else
        {
            outputChannelAutorangeStatus = HIGHEST_RANGE_LIMIT_REACHED;
        }
#if 0 // Determine when to log this once we have a log verbosity configuration
        wsprintf( fraStatusText, L"Status: Output signal over-range" );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
#endif
    }

    if (ovIn && ovOut) // If both are over-range, signal to skip further processing.
    {
        retVal = false;
        // Initialize the diagnostic record since CheckSignalRanges will not run
        inAmps[freqStepIndex][autorangeRetryCounter] = 0.0;
        outAmps[freqStepIndex][autorangeRetryCounter] = 0.0;
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
#if 0
    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];
#endif

    // Want amplitude to be above a lower threshold to avoid excessive quantization noise.
    // Want the signal to be below an upper threshold to avoid being close to overflow.

    // Check Input
    if (inputChannelAutorangeStatus == OK)
    {
        if (((double)inputAbsMax[freqStepIndex][autorangeRetryCounter]/rangeCounts) > maxAmplitudeRatio)
        {
            if (currentInputChannelRange < inputMaxRange)
            {
                currentInputChannelRange = (PS_RANGE)((int)currentInputChannelRange + 1);
                inputChannelAutorangeStatus = AMPLITUDE_TOO_HIGH;
            }
            else
            {
                inputChannelAutorangeStatus = OK; // Can't go any higher, but OK to stay here
            }
            retVal = false;
        }
        else if (((double)inputAbsMax[freqStepIndex][autorangeRetryCounter]/rangeCounts) <
                 (maxAmplitudeRatio/rangeInfo[currentInputChannelRange].ratioDown - minAmplitudeRatioTolerance))
        {
            if (currentInputChannelRange > inputMinRange)
            {
                currentInputChannelRange = (PS_RANGE)((int)currentInputChannelRange - 1);
                inputChannelAutorangeStatus = AMPLITUDE_TOO_LOW;
                retVal = false;
            }
            else
            {
                if (((double)inputAbsMax[freqStepIndex][autorangeRetryCounter]/rangeCounts) < minAllowedAmplitudeRatio)
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
#if 0 // Determine when to log this once we have a log verbosity configuration
        swprintf( fraStatusText, 128, L"Status: Input absolute peak: %hu counts", inputAbsMax[freqStepIndex][autorangeRetryCounter] );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
#endif
    }
    else
    {
        retVal = false; // Still need to adjust for the overflow on this channel
    }

    // Check Output
    if (outputChannelAutorangeStatus == OK)
    {
        if (((double)outputAbsMax[freqStepIndex][autorangeRetryCounter]/rangeCounts) > maxAmplitudeRatio)
        {
            if (currentOutputChannelRange < outputMaxRange)
            {
                currentOutputChannelRange = (PS_RANGE)((int)currentOutputChannelRange + 1);
                outputChannelAutorangeStatus = AMPLITUDE_TOO_HIGH;
            }
            else
            {
                outputChannelAutorangeStatus = OK; // Can't go any higher, but OK to stay here
            }
            retVal = false;
        }
        else if (((double)outputAbsMax[freqStepIndex][autorangeRetryCounter]/rangeCounts) <
                 (maxAmplitudeRatio/rangeInfo[currentOutputChannelRange].ratioDown - minAmplitudeRatioTolerance))
        {
            if (currentOutputChannelRange > outputMinRange)
            {
                currentOutputChannelRange = (PS_RANGE)((int)currentOutputChannelRange - 1);
                outputChannelAutorangeStatus = AMPLITUDE_TOO_LOW;
                retVal = false;
            }
            else
            {
                if (((double)outputAbsMax[freqStepIndex][autorangeRetryCounter]/rangeCounts) < minAllowedAmplitudeRatio)
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
#if 0 // Determine when to log this once we have a log verbosity configuration
        swprintf( fraStatusText, 128, L"Status: Output absolute peak: %hu counts", outputAbsMax[freqStepIndex][autorangeRetryCounter] );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
#endif
    }
    else
    {
        retVal = false; // Still need to adjust for the overflow on this channel
    }

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
//    [out] unwrappedPhasesDeg - array of unwrapped phase shifts at each frequency point of freqsLogHz, 
//          expressed in degrees
//
// Notes: The memory returned in the pointers is only valid until the next FRA execution or
//        destruction of the PicoScope FRA object.  If there is no valid data, numSteps
//        is set to 0.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::GetResults( int* numSteps, double** freqsLogHz, double** gainsDb, double** phasesDeg, double** unwrappedPhasesDeg )
{
    *numSteps = latestCompletedNumSteps;
    *freqsLogHz = latestCompletedFreqsLogHz.data();
    *gainsDb = latestCompletedGainsDb.data();
    *phasesDeg = latestCompletedPhasesDeg.data();
    *unwrappedPhasesDeg = latestCompletedUnwrappedPhasesDeg.data();
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
    latestCompletedUnwrappedPhasesDeg = unwrappedPhasesDeg;
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
//        "patches up" the results to align to frequencies the scope is capable of
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
    unwrappedPhasesDeg.resize(numSteps);

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
        freqsHz[i] = ps->GetClosestSignalGeneratorFrequency( freqsHz[i] );
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

    idealStimulusVpp.resize(numSteps);

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

    diagNumSamplesToPlot.resize(numSteps);
    diagNumStimulusCyclesCaptured.resize(numSteps);
    diagNumSamplesCaptured.resize(numSteps);
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
    stringstream overallTitle;
    stringstream inputTitle;
    stringstream outputTitle;
    wstring diagDataPath;
    int maxSamples;
    vector<double> times;
    vector<double> inputMinVoltages;
    vector<double> outputMinVoltages;
    vector<double> inputMaxVoltages;
    vector<double> outputMaxVoltages;
    double inputAmplitude, outputAmplitude;
    FRA_STATUS_MESSAGE_T fraStatusMsg;    
    double maxValue;

    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, L"Status: Generating diagnostic time domain plots." );

    maxSamples = *max_element(begin(diagNumSamplesToPlot),end(diagNumSamplesToPlot));
    times.resize(maxSamples);
    inputMinVoltages.resize(maxSamples);
    outputMinVoltages.resize(maxSamples);
    if (mSamplingMode == HIGH_NOISE)
    {
        inputMaxVoltages.resize(maxSamples);
        outputMaxVoltages.resize(maxSamples);
    }

    maxValue = (double)(ps->GetMaxValue());

    // Setup and test the diag directory for use as a current directory
    diagDataPath = mBaseDataPath + L"\\diag";
    if (0 == SetCurrentDirectory( diagDataPath.c_str() ))
    {
        // Try to create it if it doesn't exist
        CreateDirectory( diagDataPath.c_str(), NULL );
        if (0 == SetCurrentDirectory( diagDataPath.c_str() ))
        {
            throw runtime_error( "Error generating diagnostic plots.  Could not find diagnostic data directory." );
        }
    }

    for( int il = 0; il < numSteps; il++)
    {
        for (int jl = 0; jl < autoRangeTries[il]; jl++ )
        {
            fileName.clear();
            fileName.str("");
            overallTitle.clear();
            overallTitle.str("");
            inputTitle.clear();
            inputTitle.str("");
            outputTitle.clear();
            outputTitle.str("");

            plsdev( "svg" );
            plsexit(HandlePLplotError);
            fileName << "step" << (il+1) << "try" << (jl+1) << ".svg";
            plsfnam( fileName.str().c_str() );

            plstar( 1, 2 ); // Setup to stack the input and output plots

            // Plot the data.
            for( int kl = 0; kl < diagNumSamplesToPlot[il]; kl++)
            {
                times[kl] = ((double)kl)*sampleInterval[il];
            }
            for( int kl = 0; kl < diagNumSamplesToPlot[il]; kl++)
            {
                inputMinVoltages[kl] = ((double)inputMinData[il][jl][kl] / maxValue) * rangeInfo[inRange[il][jl]].rangeVolts;
            }
            for( int kl = 0; kl < diagNumSamplesToPlot[il]; kl++)
            {
                outputMinVoltages[kl] = ((double)outputMinData[il][jl][kl] / maxValue) * rangeInfo[outRange[il][jl]].rangeVolts;
            }
            if (mSamplingMode == HIGH_NOISE)
            {
                for( int kl = 0; kl < diagNumSamplesToPlot[il]; kl++)
                {
                    inputMaxVoltages[kl] = ((double)inputMaxData[il][jl][kl] / maxValue) * rangeInfo[inRange[il][jl]].rangeVolts;
                }
                for( int kl = 0; kl < diagNumSamplesToPlot[il]; kl++)
                {
                    outputMaxVoltages[kl] = ((double)outputMaxData[il][jl][kl] / maxValue) * rangeInfo[outRange[il][jl]].rangeVolts;
                }
            }

            // Plot input
            plenv( 0.0, diagNumSamplesToPlot[il]*sampleInterval[il],
                   -rangeInfo[inRange[il][jl]].rangeVolts, rangeInfo[inRange[il][jl]].rangeVolts,
                   0, 0 );
            plcol0(1);
            // Need second condition because pljoin won't place a point when the two points to join are the same.
            // The two points will be the same when in high noise mode and the number of samples are less than
            // the time domain diagnostic data limit (i.e. no aggregation was necessary).
            if (mSamplingMode == LOW_NOISE || diagNumSamplesToPlot[il] <= timeDomainDiagnosticDataLengthLimit)
            {
                plline( diagNumSamplesToPlot[il], times.data(), inputMinVoltages.data() );
            }
            else
            {
                for (int kl = 0; kl < diagNumSamplesToPlot[il]; kl++)
                {
                    pljoin( times[kl], inputMinVoltages[kl], times[kl], inputMaxVoltages[kl] );
                }
            }

            // Draw input amplitude lines if no overflow
            plcol0(2);
            if (!inOV[il][jl])
            {
                inputAmplitude = (inAmps[il][jl] / maxValue) * rangeInfo[inRange[il][jl]].rangeVolts;
                pljoin( 0.0, inputAmplitude, diagNumSamplesToPlot[il]*sampleInterval[il], inputAmplitude);
                pljoin( 0.0, -inputAmplitude, diagNumSamplesToPlot[il]*sampleInterval[il], -inputAmplitude);
                inputTitle << "Input signal; Amplitude: " << setprecision(6) << inputAmplitude << " V;" << "Purity: " << setprecision(6) << inputPurity[il][jl];
            }
            else
            {
                inputTitle << "Input signal; Overrange";
            }

            plcol0(1);
            pllab( "Time (s)", "Volts", inputTitle.str().c_str() );

            // Add an overall title, making its position relative to the top plot
            overallTitle.precision(3); // To record frequency the same as the status logs
            overallTitle << fixed;
            overallTitle << "Step " << (il+1) << ", Try " << (jl+1) << "; Frequency: " << freqsHz[il]
                         << ", Stimulus Cycles: " << diagNumStimulusCyclesCaptured[il]
                         << ", Samples Captured: " << diagNumSamplesCaptured[il];

            plmtex("t", 4.0, 0.5, 0.5, overallTitle.str().c_str());

            // Plot output
            plenv( 0.0, diagNumSamplesToPlot[il]*sampleInterval[il],
                   -rangeInfo[outRange[il][jl]].rangeVolts, rangeInfo[outRange[il][jl]].rangeVolts,
                   0, 0 );
            plcol0(1);

            // Need second condition because pljoin won't place a point when the two points to join are the same.
            // The two points will be the same when in high noise mode and the number of samples are less than
            // the time domain diagnostic data limit (i.e. no aggregation was necessary).
            if (mSamplingMode == LOW_NOISE || diagNumSamplesToPlot[il] <= timeDomainDiagnosticDataLengthLimit)
            {
                plline( diagNumSamplesToPlot[il], times.data(), outputMinVoltages.data() );
            }
            else
            {
                for (int kl = 0; kl < diagNumSamplesToPlot[il]; kl++)
                {
                    pljoin( times[kl], outputMinVoltages[kl], times[kl], outputMaxVoltages[kl] );
                }
            }

            // Draw output amplitude lines if no overflow
            plcol0(2);
            if (!outOV[il][jl])
            {
                outputAmplitude = (outAmps[il][jl] / maxValue) * rangeInfo[outRange[il][jl]].rangeVolts;
                pljoin( 0.0, outputAmplitude, diagNumSamplesToPlot[il]*sampleInterval[il], outputAmplitude);
                pljoin( 0.0, -outputAmplitude, diagNumSamplesToPlot[il]*sampleInterval[il], -outputAmplitude);
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
// Name: PicoScopeFRA::HandlePLplotError
//
// Purpose: Function that gets installed to handle PLplot errors more gracefully.  Normally
//          PLplot just calls abort(), silently terminating the whole application.  We'll 
//          report the error via exceptions and handle them at a higher level.
//
// Parameters: [in] error - string containing error message from PLplot
//
// Notes: This function must not return and must throw an exception
//
///////////////////////////////////////////////////////////////////////////////////////////////////

int PicoScopeFRA::HandlePLplotError(const char* error)
{
    string plplotErrorString = "PLplot error generating diagnostic plots: ";
    plplotErrorString += error;
    plend();
    throw runtime_error( plplotErrorString.c_str() );
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

    if (autorangeRetryCounter == 0)
    {
        swprintf( fraStatusText, 128, L"Status: Setting input frequency to %0.3lf Hz", measFreqHz );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
    }

    if (mAdaptiveStimulus)
    {
        if (0 == adaptiveStimulusRetryCounter)
        {
            // Compute the initial stimulus Vpp since this is the first attempt at this frequency
            CalculateStepInitialStimulusVpp();
            stimulusChanged = true;
        }
        swprintf( fraStatusText, 128, L"Status: Setting input stimulus to %0.3lf Vpp", currentStimulusVpp );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
    }

    if (autorangeRetryCounter == 0 || stimulusChanged)
    {
        if (!(ps->SetSignalGenerator((float)currentStimulusVpp, (float)measFreqHz)))
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

    autoStimulusInputChannelRange = currentInputChannelRange;
    autoStimulusOutputChannelRange = currentOutputChannelRange;

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
        numCycles = min( 100, (uint32_t)((measFreqHz * (double)maxScopeSamplesPerChannel) / ps->GetNoiseRejectModeSampleRate()) );
        numSamples = (int32_t)(((double)numCycles * ps->GetNoiseRejectModeSampleRate()) / measFreqHz) + 1;

        timebase = ps->GetNoiseRejectModeTimebase();
        actualSampFreqHz = ps->GetNoiseRejectModeSampleRate();
    }

    if (mDiagnosticsOn)
    {
        diagNumStimulusCyclesCaptured[freqStepIndex] = numCycles;
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

#if defined(WORKAROUND_PS_TIMEINDISPOSED_BUG)
    timeIndisposedMs = (int32_t)(((double)numSamples / actualSampFreqHz)*1000.0);
#endif

    swprintf( fraStatusText, 128, L"Status: Capturing %d samples (%d cycles) at %.3lg Hz takes %0.1lf sec.", numSamples, numCycles, actualSampFreqHz, (double)timeIndisposedMs/1000.0 );
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::ProcessData
//
// Purpose: Retrieve the data the scope has captured and run signal processing
//
// Parameters: [out] return - Whether the function was successful.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::ProcessData(void)
{
    bool retVal = true;
    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];

    wsprintf( fraStatusText, L"Status: Transferring and processing %d samples", numSamples );
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );

    if (!(ps->GetPeakValues( inputAbsMax[freqStepIndex][autorangeRetryCounter], outputAbsMax[freqStepIndex][autorangeRetryCounter], ovIn, ovOut )))
    {
        throw FraFault();
    }
    else if (false == CheckSignalOverflows())
    {
        // Both channels are over-range, don't bother with further analysis.
        retVal = false; // Signal to try again on a different range
        autorangeRetryCounter++;
    }
    else if (false == CheckSignalRanges())
    {
        // At least one of the channels needs adjustment
        retVal = false; // Signal to try again on a different range
        autorangeRetryCounter++;
    }

    // Run signal processing
    // 1) If both signal's ranges are acceptable
    //    OR
    // 2) If diagnostics are on and at least one of the signal's range is acceptable
    //    OR
    // 3) If adaptive stimulus is on and at least one of the signal's range is acceptable
    if (retVal == true || ((mDiagnosticsOn || mAdaptiveStimulus) &&
                           (CHANNEL_OVERFLOW != inputChannelAutorangeStatus ||
                            CHANNEL_OVERFLOW != outputChannelAutorangeStatus)))
    {
        uint32_t currentSampleIndex = 0;
        uint32_t maxDataRequestSize = ps->GetMaxDataRequestSize();
        uint32_t numSamplesToFeed;
        double inputAmplitude, outputAmplitude;
        
        InitGoertzel( numSamples, actualSampFreqHz, currentFreqHz );
        
        for (currentSampleIndex = 0; currentSampleIndex < numSamples; currentSampleIndex+=numSamplesToFeed)
        {
            numSamplesToFeed = min(maxDataRequestSize, numSamples-currentSampleIndex);

            if (false == ps->GetData( numSamplesToFeed, currentSampleIndex, &pInputBuffer, &pOutputBuffer ))
            {
                throw FraFault();
            }
            else
            {
                FeedGoertzel(pInputBuffer->data(), pOutputBuffer->data(), numSamplesToFeed);
            }
        }

        GetGoertzelResults( currentInputMagnitude, currentInputPhase, inputAmplitude, currentInputPurity, 
                            currentOutputMagnitude, currentOutputPhase, outputAmplitude, currentOutputPurity );

        if (mDiagnosticsOn)
        {
            // Make records for diagnostics
            inAmps[freqStepIndex][autorangeRetryCounter] = inputAmplitude;
            outAmps[freqStepIndex][autorangeRetryCounter] = outputAmplitude;
            inputPurity[freqStepIndex][autorangeRetryCounter] = currentInputPurity;
            outputPurity[freqStepIndex][autorangeRetryCounter] = currentOutputPurity;
        }

        if (mAdaptiveStimulus)
        {
            // Using the range indices recorded before auto-ranging, because auto-ranging could have changed the current index
            currentInputAmplitudeVolts = (inputAmplitude / (double)(ps->GetMaxValue())) *
                                         rangeInfo[autoStimulusInputChannelRange].rangeVolts *
                                         attenInfo[mInputChannelAttenuation];
            currentOutputAmplitudeVolts = (outputAmplitude / (double)(ps->GetMaxValue())) *
                                          rangeInfo[autoStimulusOutputChannelRange].rangeVolts *
                                          attenInfo[mOutputChannelAttenuation];

            swprintf( fraStatusText, 128, L"Status: Input amplitude: %0.3lf V", currentInputAmplitudeVolts );
            UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );
            swprintf( fraStatusText, 128, L"Status: Output amplitude: %0.3lf V", currentOutputAmplitudeVolts );
            UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText );

            if (false == CheckStimulusTarget())
            {
                retVal = false;
            }
            else if (CHANNEL_OVERFLOW != inputChannelAutorangeStatus && CHANNEL_OVERFLOW != outputChannelAutorangeStatus)
            {
                CheckStimulusTarget(true);
                idealStimulusVpp[freqStepIndex] = currentStimulusVpp;
            }
        }
    }
    if (mDiagnosticsOn)
    {
        uint32_t compressedSize = mSamplingMode == LOW_NOISE ? 0 : timeDomainDiagnosticDataLengthLimit;
        ps->GetCompressedData( compressedSize, inputMinData[freqStepIndex][autorangeRetryCounter],
                                               outputMinData[freqStepIndex][autorangeRetryCounter],
                                               inputMaxData[freqStepIndex][autorangeRetryCounter],
                                               outputMaxData[freqStepIndex][autorangeRetryCounter] );
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::CheckStimulusTarget
//
// Purpose: Determine whether the stimulus amplitude needs to change for adaptive stimulus mode
//
// Parameters: [in] forceAdjust - if true, re-calculate regardless of whether the input or
//                                output amplitude are acceptable (within target + tolerance).
//                                useful for one final calculation of ideal stimulus.
//                                Defaults to false.
//             [out] return - false if the stimulus needs to change
//
// Notes: Strategy is to get the smaller signal within margin of target
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::CheckStimulusTarget(bool forceAdjust)
{
    double newStimulusFromInput = 0.0;
    double newStimulusFromOutput = 0.0;
    int inputRelation = 0; // -1 => under target; 1 => over target; 0 => within target
    int outputRelation = 0; // -1 => under target; 1 => over target; 0 => within target

    // Only bother checking on channels that are not over-range
    if (CHANNEL_OVERFLOW != inputChannelAutorangeStatus)
    {
        if (currentInputAmplitudeVolts < mTargetResponseAmplitude)
        {
            inputRelation = -1;
        }
        else if (currentInputAmplitudeVolts > (1.0 + mTargetResponseAmplitudeTolerance) * mTargetResponseAmplitude)
        {
            inputRelation = 1;
        }
        else
        {
            inputRelation = 0;
        }

        if (0 != inputRelation || forceAdjust)
        {
            // Calculate new value
            newStimulusFromInput = currentStimulusVpp * (((1.0 + mTargetResponseAmplitudeTolerance / 2.0) * mTargetResponseAmplitude) / currentInputAmplitudeVolts);
        }
    }
    // else - just leave inputRelation as "OK" since auto-ranging will cause a retry
    if (CHANNEL_OVERFLOW != outputChannelAutorangeStatus)
    {
        if (currentOutputAmplitudeVolts < mTargetResponseAmplitude)
        {
            outputRelation = -1;
        }
        else if (currentOutputAmplitudeVolts > (1.0 + mTargetResponseAmplitudeTolerance) * mTargetResponseAmplitude)
        {
            outputRelation = 1;
        }
        else
        {
            outputRelation = 0;
        }

        if (0 != outputRelation || forceAdjust)
        {
            // Calculate new value
            newStimulusFromOutput = currentStimulusVpp * (((1.0 + mTargetResponseAmplitudeTolerance / 2.0) * mTargetResponseAmplitude) / currentOutputAmplitudeVolts);
        }
    }
    // else - just leave outputRelation as "OK" since auto-ranging will cause a retry

    if (((inputRelation == 0 && outputRelation != -1) || (outputRelation == 0 && inputRelation != -1)) && !forceAdjust)
    {
        return true;
    }
    else
    {
        adaptiveStimulusRetryCounter += forceAdjust ? 0 : 1;
        // Bound the result.  Can't be higher than function generator maximum.  Need to avoid 0.0 or else future
        // adjustment will be bound to 0.0.
        currentStimulusVpp = max(ps->GetMinNonZeroFuncGenVpp(), min(mMaxStimulusVpp, max(newStimulusFromInput, newStimulusFromOutput)));
        return forceAdjust;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::CalculateStepInitialStimulusVpp
//
// Purpose: Compute the initial value of stimulus amplitude for the new frequency for adaptive
//          stimulus mode
//
// Parameters: None
//
// Notes: N/A
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::CalculateStepInitialStimulusVpp(void)
{
    if (freqStepCounter > 2)
    {
        // Two prior values exist, so estimate new stimulus Vpp based on their slope
        uint32_t idxMinus1, idxMinus2;
        idxMinus1 = mSweepDescending ? freqStepIndex + 1 : freqStepIndex - 1;
        idxMinus2 = mSweepDescending ? freqStepIndex + 2 : freqStepIndex - 2;
        currentStimulusVpp += ((freqsHz[freqStepIndex] - freqsHz[idxMinus1]) *
                               (idealStimulusVpp[idxMinus1] - idealStimulusVpp[idxMinus2])) /
                               (freqsHz[idxMinus1] - freqsHz[idxMinus2]);
        // Bound the result.  Can't be higher than function generator maximum.  Need to avoid 0.0 or else future
        // adjustment will be bound to 0.0.
        currentStimulusVpp = min(mMaxStimulusVpp, max(ps->GetMinNonZeroFuncGenVpp(), currentStimulusVpp));
    }
    // if freqStepCounter is 1, then just start with the initialized value
    // if freqStepCounter is 2, then just keep the prior value.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::CalculateGainAndPhase
//
// Purpose: Calculate gain and phase shift of the input and output signals.
//
// Parameters: [out] gain - gain of input over output
//             [out] phase - phase shift from input to output
//             [out] return - Whether the function was successful.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////
 
bool PicoScopeFRA::CalculateGainAndPhase( double* gain, double* phase)
{
    double tempPhase;
    double crossover, crossoverUpper, crossoverLower;

    // Compute gain as dB
    // Compute channel range gain factor
    double channelGainFactor = (rangeInfo[currentOutputChannelRange].rangeVolts * attenInfo[mOutputChannelAttenuation]) / 
                               (rangeInfo[currentInputChannelRange].rangeVolts * attenInfo[mInputChannelAttenuation]);
    *gain = 20.0 * log10( channelGainFactor * currentOutputMagnitude / currentInputMagnitude );

    // Compute phase in degrees, crossing over at the designated crossover
    crossover = M_PI * (mPhaseWrappingThreshold / 180.0);

    if (crossover < 0.0)
    {
        crossoverLower = crossover;
        crossoverUpper = crossoverLower + 2.0*M_PI; 
    }
    else
    {
        crossoverUpper = crossover;
        crossoverLower = crossoverUpper - 2.0*M_PI; 
    }

    tempPhase = currentOutputPhase - currentInputPhase;
    if (tempPhase > crossoverUpper)
    {
        tempPhase -= 2.0*M_PI;
    }
    else if (tempPhase < crossoverLower)
    {
        tempPhase += 2.0*M_PI;
    }

    *phase = tempPhase*180.0/M_PI;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::UnwrapPhases
//
// Purpose: Take raw phase values and remove jumps in phase.
//
// Parameters: [out] phasesDeg - raw phases to unwrap
//             [out] unwrappedPhasesDeg - unwrapped phases
//
// Notes: Also providing implementation of remainder since it's not available in  MSVC2012 and
//        not availble in Boost without a compiled library
//
///////////////////////////////////////////////////////////////////////////////////////////////////

double remainder( double numer, double denom )
{
    double rquot = boost::math::round( numer / denom );
    return (numer - rquot * denom);
}
void PicoScopeFRA::UnwrapPhases(void)
{
    // Copy over the raw phases
    unwrappedPhasesDeg = phasesDeg;

    std::vector<double>::iterator unwrappedIt = unwrappedPhasesDeg.begin();

    for (unwrappedIt++; unwrappedIt != unwrappedPhasesDeg.end(); unwrappedIt++)
    {
        double oldJump = *unwrappedIt - *std::prev(unwrappedIt);
        if(fabs(oldJump) > 180.0)
        {
            double newJump = remainder(oldJump, 360.0);
            *unwrappedIt = *std::prev(unwrappedIt) + newJump;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::XXXXGoertzel
//
// Purpose: The Goertzel algorithm is a fast method of computing a single point DFT.  Both magnitude and phase
// are returned.  The advantage of performing the Goertzel algorithm vs an FFT is that for our application we're only
// interested in a single frequency per measurement, and thus Goertzel is faster than FFT.  Also, an FFT requires the
// full sample buffer to be stored.  Without an additional decimation, storing the whole sample buffer would be highly
// impractical for some PicoScopes (up to 4GB of RAM needed).  This implementation is a generalized Goertzel algorithm
// which allows for a non-integer bin (k ∈ R), and thus is really a single point DTFT.  An important advantage of the
// generalized Goertzel is that we don't have to adjust the number of samples or sampling rate to maintain accuracy of
// the frequency selection.  These functions also compute other useful parameters such as amplitude and purity, which
// can be used for data quality decisions.
//
// Parameters: 
//    InitGoertzel
//             [in] totalSamples - Total number of samples in the full signal
//             [in] fSamp - frequency of the sampling
//             [in] fDetect - frequency to detect
//    FeedGoertzel
//             [in] inputSamples - input channel data sample points
//             [in] outputSamples - output channel data sample points
//             [in] n - number of samples in this block of samples
//    GetGoertzelResults
//             [output] [in/out]putMagnitude - The Goertzel magnitude
//             [output] [in/out]putPhase - The Goertzel phase
//             [output] [in/out]putAmplitude - The measured amplitude of the signal
//             [output] [in/out]putPurity - The purity of the signal (signal power over total power)
//
// Notes: 
//
// - The following reference derives the (k ∈ R) generalized Goertzel:
//          doi:10.1186/1687-6180-2012-56
//          Sysel and Rajmic:
//          Goertzel algorithm generalized to non-integer multiples of fundamental frequency.
//          EURASIP Journal on Advances in Signal Processing 2012 2012:56.
//
// - The standard recurrence used by the original Goertzel is known to have numerical stability
//   issues - i.e. O(N^2) error growth for small k.  This can start to be a real problem for scopes with very
//   large sample buffers (e.g. currently up to 1GS on the 6000 series).  The Reinsch modification (recurrence)
//   is a well known modification to deal with error near k=0.  As k/N approaches pi/2, the Reinsch recurrence
//   numerical accuracy becomes worse than Goertzel (Oliver 77).  However, for our application when k is
//   significantly larger than 0, N is also small so as to minimize error concerns.  So, we can use Reinsch
//   all the time and get good error performance.
//
// - The d.c. energy calculation is an execution of the Goertzel with Magic Circle Oscillator.  When the
//   tuning frequency is 0Hz, the tuning parameter is 0.  Thus the energy calculation is simplified.
//    - The idea to use alternate oscillators comes from the work of Clay Turner on Oscillator theory and
//      application to the Goertzel:
//      Ref. http://www.claysturner.com/dsp/digital_resonators.pdf
//      Ref. http://www.claysturner.com/dsp/ResonatorTable.pdf
//
// - Magic Circle could also be used for the main signal detecting Goertzel, but its speed performance is a little
//   worse and numerical accuracy only a little better for sample buffers up to 1GS.  Its speed performance might be
//   able to become on-par with Reinsch on a more modern SIMD unit.
//
// - In experimenting with different oscillators, it was found that the (k ∈ R) phase correction factor can
//   differ from e^j*2pi*k, but it still remains dependent only on k.  However, since this application is only
//   interested in phase shift between input and output signals, and these have the same frequency (k), we need
//   not apply the (k ∈ R) phase correction factor.  So, this implementation does not produce a "mathematically
//   correct" phase, but that's OK for our application.
//
// - The benefit of processing the input and output signals together is that we can take advantage of SIMD 
//   parallelism (SSE, AVX, etc);  However, since the MSVC auto-vectorizer seems unable to vectorize the core
//   loop, we're going to code it with intrinsics.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Goertzel coefficient and state data
// Making these non class members to help simplify alignment
double theta;
alignas(16) static double Kappa;
alignas(16) static double a[2], b[2];
alignas(16) static double totalEnergy[2];
alignas(16) static double dcEnergy[2];
static uint32_t samplesProcessed;
static uint32_t N;
// Goertzel outputs
static array<double,2> magnitude, phase, amplitude, purity;

void PicoScopeFRA::InitGoertzel( uint32_t totalSamples, double fSamp, double fDetect )
{
    double halfTheta;
    a[0] = a[1] = b[0] = b[1] = 0.0;
    totalEnergy[0] = totalEnergy[1] = 0.0;
    dcEnergy[0] = dcEnergy[1] = 0.0;
    samplesProcessed = 0;

    N = totalSamples;

    theta = 2.0 * M_PI * (fDetect / fSamp);
    halfTheta = M_PI * (fDetect / fSamp);
    Kappa = 4.0 * pow( sin( halfTheta ), 2.0 );
}

void PicoScopeFRA::FeedGoertzel( int16_t* inputSamples, int16_t* outputSamples, uint32_t n )
{
    bool lastBlock = false;

    // Vectors
    __m128d KappaVec, aVec, bVec, totalEnergyVec, dcEnergyVec, sampleVec;

    // Determine if this is the last block.  If it is, there is special processing.
    lastBlock = ((samplesProcessed + n) == N);

    // Load vectors
    KappaVec = _mm_load1_pd(&Kappa);
    aVec = _mm_load_pd(a);
    bVec = _mm_load_pd(b);
    totalEnergyVec = _mm_load_pd(totalEnergy);
    dcEnergyVec = _mm_load_pd(dcEnergy);

    // Execute the filter, d.c. Energy and Parseval energy time domain calculation
    for (uint32_t i = 0; i < n; i++)
    {
        // Load and convert samples to packed doubles
        sampleVec = _mm_cvtepi32_pd( _mm_set_epi32( 0, 0, outputSamples[i], inputSamples[i] ) );

        //totalEnergy += samples[i]*samples[i];
        totalEnergyVec = _mm_add_pd( totalEnergyVec, _mm_mul_pd( sampleVec, sampleVec ) );

        //dcEnergy += samples[i];
        dcEnergyVec = _mm_add_pd( dcEnergyVec, sampleVec );

        //b = b - K*a + samples[i]
        //a = a + b
        bVec = _mm_add_pd( _mm_sub_pd(bVec, _mm_mul_pd(KappaVec, aVec)), sampleVec);
        aVec = _mm_add_pd(aVec, bVec);
    }

    samplesProcessed += n;

    if (lastBlock)
    {
        array<complex<double>, 2> y;
        array<double, 2> signalEnergy;

        // Iterate Goertzel once more with 0 input to get correct phase
        bVec = _mm_sub_pd(bVec, _mm_mul_pd(KappaVec, aVec));
        aVec = _mm_add_pd(aVec, bVec);

        // Unvectorize
        _mm_store_pd(a, aVec);
        _mm_store_pd(b, bVec);
        _mm_store_pd(totalEnergy, totalEnergyVec);
        _mm_store_pd(dcEnergy, dcEnergyVec);

        // Compute the complex output
        y[0] = std::complex<double>(b[0] + a[0] * (cos(theta) - 1.0), a[0] * sin(theta));
        y[1] = std::complex<double>(b[1] + a[1] * (cos(theta) - 1.0), a[1] * sin(theta));

        magnitude[0] = abs(y[0]);
        magnitude[1] = abs(y[1]);
        phase[0] = arg(y[0]);
        phase[1] = arg(y[1]);

        // Using N+1 because this form of the Goertzel iterates N+1 times, with x[N] = 0, thus effectively using N+1 samples.
        // The x[N]=0 sample has no effect on the time domain Parseval's energy calculation.
        amplitude[0] = 2.0 * magnitude[0] / (N+1);
        amplitude[1] = 2.0 * magnitude[1] / (N+1);
        signalEnergy[0] = 2.0 * (magnitude[0] * magnitude[0]) / (N+1);
        signalEnergy[1] = 2.0 * (magnitude[1] * magnitude[1]) / (N+1);
        dcEnergy[0] = (dcEnergy[0] * dcEnergy[0]) / (N+1);
        dcEnergy[1] = (dcEnergy[1] * dcEnergy[1]) / (N+1);

        purity[0] = signalEnergy[0] / (totalEnergy[0] - dcEnergy[0]);
        purity[1] = signalEnergy[1] / (totalEnergy[1] - dcEnergy[1]);
    }
    else
    {
        // Unvectorize
        _mm_store_pd(a, aVec);
        _mm_store_pd(b, bVec);
        _mm_store_pd(totalEnergy, totalEnergyVec);
        _mm_store_pd(dcEnergy, dcEnergyVec);
    }
}

void PicoScopeFRA::GetGoertzelResults( double& inputMagnitude, double& inputPhase, double& inputAmplitude, double& inputPurity,
                                       double& outputMagnitude, double& outputPhase, double& outputAmplitude, double& outputPurity )
{
    inputMagnitude = magnitude[0];
    inputPhase = phase[0];
    inputAmplitude = amplitude[0];
    inputPurity = purity[0];

    outputMagnitude = magnitude[1];
    outputPhase = phase[1];
    outputAmplitude = amplitude[1];
    outputPurity = purity[1];
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