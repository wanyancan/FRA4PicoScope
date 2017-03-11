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
const double PicoScopeFRA::stimulusBasedInitialRangeEstimateMargin = 0.95;
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
    adaptiveStimulusInputChannelRange = (PS_RANGE)0;
    adaptiveStimulusOutputChannelRange = (PS_RANGE)0;

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
    mInputStartRange = 0;
    mOutputStartRange = 0;
    mExtraSettlingTimeMs = 0;
    mMinCyclesCaptured = 0;
    mMaxDftBw = 0.0;
    mLowNoiseOversampling = 0;
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
    stepStimulusVpp = 0.0;
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
    if (NULL != (ps = _ps))
    {
        numAvailableChannels = ps->GetNumChannels();
        rangeInfo = ps->GetRangeCaps();
        signalGeneratorPrecision = ps->GetSignalGeneratorPrecision();
        rangeCounts = ps->GetMaxValue();

        // Setup arbitrary initial settings to force a calculation of maxScopeSamplesPerChannel
        SetupChannels( PS_CHANNEL_A, PS_AC, ATTEN_1X, 0.0, PS_CHANNEL_B, PS_AC, ATTEN_1X, 0.0, 0.0, 0.0 );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::SetFraSettings
//
// Purpose: Set basic setting that are optional to set, but may need to be set occassionally
//
// Parameters: [in] samplingMode - Low or high noise sampling mode
//             [in] adaptiveStimulusMode - if true, run in adaptive stimulus mode
//             [in] targetResponseAmplitude - target for amplitude of the response signals; don't
//                                            allow either of input or output be less than this.
//             [in] sweepDescending - if true, sweep from highest frequency to lowest
//             [in] phaseWrappingThreshold - phase value to use as wrapping point (in degrees)
//                                           absolute value should be less than 360
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::SetFraSettings( SamplingMode_T samplingMode, bool adaptiveStimulusMode, double targetResponseAmplitude,
                                   bool sweepDescending, double phaseWrappingThreshold )
{
    mSamplingMode = samplingMode;
    mAdaptiveStimulus = adaptiveStimulusMode;
    mTargetResponseAmplitude = targetResponseAmplitude;
    mSweepDescending = sweepDescending;
    mPhaseWrappingThreshold = phaseWrappingThreshold;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::SetFraTuning
//
// Purpose: Set more advanced settings that are optional to set, but may need to be set rarely
//
// Parameters: [in] purityLowerLimit - Lower limit on purity before we take action
//             [in] extraSettlingTimeMs - additional settling time to insert between setting up
//                                         signal generator and sampling
//             [in] autorangeTriesPerStep - Number of range tries allowed
//             [in] autorangeTolerance - Hysterysis used to determine when the switch
//             [in] smallSignalResolutionTolerance - Lower limit on signal amplitide before we
//                                                    take action
//             [in] maxAutorangeAmplitude - Amplitude before we switch to next higher range
//             [in] inputStartRange - Range to start input channel; -1 means base on stimulus
//             [in] outputStartRange - Range to start output channel; -1 means base on stimulus
//             [in] adaptiveStimulusTriesPerStep - Number of adaptive stimulus tries allowed
//             [in] targetResponseAmplitudeTolerance - Percent tolerance above target allowed for
//                                                     the smallest stimulus (input or output)
//             [in] minCyclesCaptured - Minimum cycles captured for stimulus signal
//             [in] maxDftBw - Maximum bandwidth for DFT in noise reject mode
//             [in] lowNoiseOversampling - Amount to oversample the stimulus frequency in low
//                                         noise mode
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::SetFraTuning( double purityLowerLimit, uint16_t extraSettlingTimeMs, uint8_t autorangeTriesPerStep,
                                 double autorangeTolerance, double smallSignalResolutionTolerance, double maxAutorangeAmplitude,
                                 int32_t inputStartRange, int32_t outputStartRange, uint8_t adaptiveStimulusTriesPerStep,
                                 double targetResponseAmplitudeTolerance, uint16_t minCyclesCaptured, double maxDftBw,
                                 uint16_t lowNoiseOversampling )
{
    mPurityLowerLimit = purityLowerLimit;
    mExtraSettlingTimeMs = extraSettlingTimeMs;
    maxAutorangeRetries = autorangeTriesPerStep;
    minAmplitudeRatioTolerance = autorangeTolerance;
    minAllowedAmplitudeRatio = smallSignalResolutionTolerance;
    maxAmplitudeRatio = maxAutorangeAmplitude;
    mInputStartRange = inputStartRange;
    mOutputStartRange = outputStartRange;
    maxAdaptiveStimulusRetries = adaptiveStimulusTriesPerStep;
    mTargetResponseAmplitudeTolerance = targetResponseAmplitudeTolerance;
    mMinCyclesCaptured = minCyclesCaptured;
    mMaxDftBw = maxDftBw;
    mLowNoiseOversampling = lowNoiseOversampling;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::EnableDiagnostics
//
// Purpose: Turn on time domain diagnostic plot output
//
// Parameters: [in] baseDataPath - where to put the "diag" directory, where the plot files will
//                                 be stored
//
// Notes: In the future this could be used to output diagnostic info to the log
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PicoScopeFRA::EnableDiagnostics(wstring baseDataPath)
{
    mDiagnosticsOn = true;
    mBaseDataPath = baseDataPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PicoScopeFRA::DisableDiagnostics
//
// Purpose: Turn off time domain diagnostic plot output
//
// Parameters: N/A
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

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
    if (ps)
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
    else
    {
        return 0.0;
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
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool PicoScopeFRA::SetupChannels( int inputChannel, int inputChannelCoupling, int inputChannelAttenuation, double inputDcOffset,
                                  int outputChannel, int outputChannelCoupling, int outputChannelAttenuation, double outputDcOffset,
                                  double initialStimulusVpp, double maxStimulusVpp )
{
    FRA_STATUS_MESSAGE_T fraStatusMsg;

    if (!ps)
    {
        return false;
    }

    mInputChannelCoupling = (PS_COUPLING)inputChannelCoupling;
    mOutputChannelCoupling = (PS_COUPLING)outputChannelCoupling;

    inputMinRange = ps->GetMinRange(mInputChannelCoupling);
    inputMaxRange = ps->GetMaxRange(mInputChannelCoupling);
    outputMinRange = ps->GetMinRange(mOutputChannelCoupling);
    outputMaxRange = ps->GetMaxRange(mOutputChannelCoupling);

    if (-1 == mInputStartRange)
    {
        for (currentInputChannelRange = inputMaxRange; currentInputChannelRange > inputMinRange; currentInputChannelRange--)
        {
            if (initialStimulusVpp > 2.0*rangeInfo[currentInputChannelRange].rangeVolts*attenInfo[mInputChannelAttenuation]*stimulusBasedInitialRangeEstimateMargin)
            {
                currentInputChannelRange++; // We stepped one too far, so backup
                currentInputChannelRange = min(currentInputChannelRange, inputMaxRange); // Just in case, so we can't get an illegal range
                break;
            }
        }
    }
    else
    {
        currentInputChannelRange = min(inputMaxRange, max(mInputStartRange, inputMinRange));
    }

    if (-1 == mOutputStartRange)
    {
        for (currentOutputChannelRange = outputMaxRange; currentOutputChannelRange > outputMinRange; currentOutputChannelRange--)
        {
            if (initialStimulusVpp > 2.0*rangeInfo[currentOutputChannelRange].rangeVolts*attenInfo[mOutputChannelAttenuation]*stimulusBasedInitialRangeEstimateMargin)
            {
                currentOutputChannelRange++; // We stepped one too far, so backup
                currentOutputChannelRange = min(currentOutputChannelRange, outputMaxRange); // Just in case, so we can't get an illegal range
                break;
            }
        }
    }
    else
    {
        currentOutputChannelRange = min(outputMaxRange, max(mOutputStartRange, outputMinRange));
    }

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
            totalRetryCounter[freqStepIndex] = 0;
            currentFreqHz = freqsHz[freqStepIndex];

            swprintf(fraStatusText, 128, L"Status: Starting frequency step %d (%0.3lf Hz)", freqStepCounter, currentFreqHz);
            UpdateStatus(fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, FRA_PROGRESS);

            for (autorangeRetryCounter = 0, adaptiveStimulusRetryCounter = 0;
                 autorangeRetryCounter < maxAutorangeRetries && adaptiveStimulusRetryCounter < maxAdaptiveStimulusRetries;)
            {
                try
                {
                    if (mAdaptiveStimulus)
                    {
                        wsprintf(fraStatusText, L"Status: Starting frequency step %d, range try %d, adaptive stimulus try %d", freqStepCounter, autorangeRetryCounter + 1, adaptiveStimulusRetryCounter + 1);
                    }
                    else
                    {
                        wsprintf(fraStatusText, L"Status: Starting frequency step %d, range try %d", freqStepCounter, autorangeRetryCounter + 1);
                    }
                    UpdateStatus(fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, STEP_TRIAL_PROGRESS);
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
                                totalRetryCounter[freqStepIndex]++; // record the attempt
                                continue; // Try again on a different range
                            }
                            else // Data is good, calculate and move on to next frequency
                            {
                                // Currently no error is possible so just cast to void
                                (void)CalculateGainAndPhase(&gainsDb[freqStepIndex], &phasesDeg[freqStepIndex]);

                                // Notify progress
                                UpdateStatus(fraStatusMsg, FRA_STATUS_IN_PROGRESS, freqStepCounter, numSteps);

                                totalRetryCounter[freqStepIndex]++; // record the attempt
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
                        // Start the step over again
                        autorangeRetryCounter = 0;
                        adaptiveStimulusRetryCounter = 0;
                        totalRetryCounter[freqStepIndex] = 0;
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
            }

            if (autorangeRetryCounter == maxAutorangeRetries ||
                adaptiveStimulusRetryCounter == maxAdaptiveStimulusRetries)
            {
                // This is a temporary solution until we implement a fully interactive one.
                UpdateStatus( fraStatusMsg, FRA_STATUS_RETRY_LIMIT, inputChannelAutorangeStatus, outputChannelAutorangeStatus );
                if (true == fraStatusMsg.responseData.proceed)
                {
                    if (fraStatusMsg.responseData.retry)
                    {
                        // Start the step over again
                        continue; // bypasses step index and counter updates
                    }
                    else // continue to next step
                    {
                        gainsDb[freqStepIndex] = 0.0;
                        phasesDeg[freqStepIndex] = 0.0;
                        // TODO - mark as invalid;
                        // Notify progress
                        UpdateStatus( fraStatusMsg, FRA_STATUS_IN_PROGRESS, freqStepCounter, numSteps );
                    }
                }
                else
                {
                    // Notify of cancellation
                    UpdateStatus( fraStatusMsg, FRA_STATUS_CANCELED, freqStepCounter, numSteps );
                    throw FraFault();
                }
            }

            // Step index and counter updates
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
    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];

    // Reset to default
    inputChannelAutorangeStatus = OK;
    outputChannelAutorangeStatus = OK;

    // Make records for diagnostics
    inOV[freqStepIndex][totalRetryCounter[freqStepIndex]] = ovIn;
    outOV[freqStepIndex][totalRetryCounter[freqStepIndex]] = ovOut;
    inRange[freqStepIndex][totalRetryCounter[freqStepIndex]] = currentInputChannelRange;
    outRange[freqStepIndex][totalRetryCounter[freqStepIndex]] = currentOutputChannelRange;

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
        wsprintf( fraStatusText, L"Status: Measured input signal over-range" );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, AUTORANGE_DIAGNOSTICS );
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
        wsprintf( fraStatusText, L"Status: Measured output signal over-range" );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, AUTORANGE_DIAGNOSTICS );
    }

    if (ovIn && ovOut) // If both are over-range, signal to skip further processing.
    {
        retVal = false;
        // Initialize the diagnostic record since CheckSignalRanges will not run
        inAmps[freqStepIndex][totalRetryCounter[freqStepIndex]] = 0.0;
        outAmps[freqStepIndex][totalRetryCounter[freqStepIndex]] = 0.0;
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
    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];

    // Want amplitude to be above a lower threshold to avoid excessive quantization noise.
    // Want the signal to be below an upper threshold to avoid being close to overflow.

    // Check Input
    if (inputChannelAutorangeStatus == OK)
    {
        if (((double)inputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]]/rangeCounts) > maxAmplitudeRatio)
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
        else if (((double)inputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]]/rangeCounts) <
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
                if (((double)inputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]]/rangeCounts) < minAllowedAmplitudeRatio)
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
        swprintf( fraStatusText, 128, L"Status: Measured input absolute peak: %hu counts", inputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]] );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, AUTORANGE_DIAGNOSTICS );
    }
    else
    {
        retVal = false; // Still need to adjust for the overflow on this channel
    }

    // Check Output
    if (outputChannelAutorangeStatus == OK)
    {
        if (((double)outputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]]/rangeCounts) > maxAmplitudeRatio)
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
        else if (((double)outputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]]/rangeCounts) <
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
                if (((double)outputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]]/rangeCounts) < minAllowedAmplitudeRatio)
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
        swprintf( fraStatusText, 128, L"Status: Measured output absolute peak: %hu counts", outputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]] );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, AUTORANGE_DIAGNOSTICS );
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
    if (numSteps && freqsLogHz && gainsDb && phasesDeg && unwrappedPhasesDeg )
    {
        *numSteps = latestCompletedNumSteps;
        *freqsLogHz = latestCompletedFreqsLogHz.data();
        *gainsDb = latestCompletedGainsDb.data();
        *phasesDeg = latestCompletedPhasesDeg.data();
        *unwrappedPhasesDeg = latestCompletedUnwrappedPhasesDeg.data();
    }
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
    int maxTotalStepTries = maxAutorangeRetries + (mAdaptiveStimulus ? maxAdaptiveStimulusRetries : 0);

    idealStimulusVpp.resize(numSteps);

    inAmps.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inAmps[i].resize(maxTotalStepTries);
    }

    outAmps.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outAmps[i].resize(maxTotalStepTries);
    }

    inOV.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inOV[i].resize(maxTotalStepTries);
    }

    outOV.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outOV[i].resize(maxTotalStepTries);
    }

    inRange.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inRange[i].resize(maxTotalStepTries);
    }

    outRange.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outRange[i].resize(maxTotalStepTries);
    }

    stimVpp.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        stimVpp[i].resize(maxTotalStepTries);
    }

    inputMinData.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inputMinData[i].resize(maxTotalStepTries);
    }

    outputMinData.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outputMinData[i].resize(maxTotalStepTries);
    }

    inputMaxData.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inputMaxData[i].resize(maxTotalStepTries);
    }

    outputMaxData.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outputMaxData[i].resize(maxTotalStepTries);
    }

    inputAbsMax.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inputAbsMax[i].resize(maxTotalStepTries);
    }

    outputAbsMax.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outputAbsMax[i].resize(maxTotalStepTries);
    }

    inputPurity.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        inputPurity[i].resize(maxTotalStepTries);
    }

    outputPurity.resize(numSteps);
    for (i = 0; i < numSteps; i++)
    {
        outputPurity[i].resize(maxTotalStepTries);
    }

    diagNumSamplesToPlot.resize(numSteps);
    diagNumStimulusCyclesCaptured.resize(numSteps);
    diagNumSamplesCaptured.resize(numSteps);
    autoRangeTries.resize(numSteps);
    adaptiveStimulusTries.resize(numSteps);
    totalRetryCounter.resize(numSteps);
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

    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, L"Status: Generating diagnostic time domain plots.", FRA_PROGRESS );

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
        for (int jl = 0; jl < totalRetryCounter[il]; jl++ )
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
            overallTitle << fixed;
            overallTitle << "Step " << (il+1) << ", Try " << (jl+1) << "; Frequency: " << setprecision(3) << freqsHz[il]
                         << " Hz, Stimulus Vpp: " << setprecision(6) << stimVpp[il][jl]
                         << " V, Stimulus Cycles: " << diagNumStimulusCyclesCaptured[il]
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

    // Record these here as a means to keep track of the actual tries attempted
    // Because of the way the retry counters are managed, it's not possible to do it later
    autoRangeTries[freqStepIndex] = autorangeRetryCounter+1;
    adaptiveStimulusTries[freqStepIndex] = adaptiveStimulusRetryCounter+1;

    if (autorangeRetryCounter == 0)
    {
        swprintf( fraStatusText, 128, L"Status: Setting signal generator frequency to %0.3lf Hz", measFreqHz );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, SIGNAL_GENERATOR_DIAGNOSTICS );
    }

    if (mAdaptiveStimulus)
    {
        if (0 == adaptiveStimulusRetryCounter)
        {
            // Compute the initial stimulus Vpp since this is the first attempt at this frequency
            CalculateStepInitialStimulusVpp();
            stimulusChanged = true;
        }
        swprintf( fraStatusText, 128, L"Status: Setting signal generator amplitude to %0.3lf Vpp", currentStimulusVpp );
        UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, ADAPTIVE_STIMULUS_DIAGNOSTICS );
    }

    if (autorangeRetryCounter == 0 || (mAdaptiveStimulus && stimulusChanged))
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

    stimVpp[freqStepIndex][totalRetryCounter[freqStepIndex]] = currentStimulusVpp;

    wsprintf( fraStatusText, L"Status: Setting input channel range to %s", rangeInfo[currentInputChannelRange].name );
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, AUTORANGE_DIAGNOSTICS );
    if( !(ps->SetupChannel((PS_CHANNEL)mInputChannel, (PS_COUPLING)mInputChannelCoupling, currentInputChannelRange, (float)mInputDcOffset)) )
    {
        return false;
    }

    wsprintf( fraStatusText, L"Status: Setting output channel range to %s", rangeInfo[currentOutputChannelRange].name );
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, AUTORANGE_DIAGNOSTICS );
    if( !(ps->SetupChannel((PS_CHANNEL)mOutputChannel, (PS_COUPLING)mOutputChannelCoupling, currentOutputChannelRange, (float)mOutputDcOffset)) )
    {
        return false;
    }

    // Record these pre-adjustment versions here.  There is code later in step execution that needs
    // cannot use post-adjusted values.
    adaptiveStimulusInputChannelRange = currentInputChannelRange;
    adaptiveStimulusOutputChannelRange = currentOutputChannelRange;
    stepStimulusVpp = currentStimulusVpp;

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
        if (!(ps->GetTimebase(measFreqHz*(double)mLowNoiseOversampling, &actualSampFreqHz, &timebase)))
        {
            return false;
        }
        // Calculate actual sample frequency, and number of samples, collecting enough
        // samples for the configured cycles of the measured frequency.
        numCycles = mMinCyclesCaptured;
        double samplesPerCycle = actualSampFreqHz / measFreqHz;
        // Deferring the integer truncation till this point ensures
        // the least inaccuracy in hitting the integer periods criteria
        numSamples = (uint32_t)(samplesPerCycle * (double)numCycles) + 1;
    }
    else // NOISE_REJECT
    {
        uint32_t minBwSamples;
        double actualDftBw;

        timebase = ps->GetNoiseRejectModeTimebase();
        actualSampFreqHz = ps->GetNoiseRejectModeSampleRate();

        // Calculate minimum number of samples required to stay <= maximum bandwidth
        minBwSamples = min((uint32_t)ceil(actualSampFreqHz / mMaxDftBw), maxScopeSamplesPerChannel);
        // Calculate number of whole stimulus cycles required to have at least minBwSamples
        numCycles = max(mMinCyclesCaptured, (uint32_t)ceil((double)minBwSamples * measFreqHz / actualSampFreqHz));
        // Calculate actal number of samples to be taken
        numSamples = min((uint32_t)(((double)numCycles * ps->GetNoiseRejectModeSampleRate()) / measFreqHz) + 1, maxScopeSamplesPerChannel);
        // Calculate actual DFT bandwidth
        actualDftBw = actualSampFreqHz / (double)numSamples;

        if (actualDftBw > mMaxDftBw)
        {
            swprintf( fraStatusText, 128, L"WARNING: Actual DFT bandwidth (%.3lg Hz) greater than requested (%.3lg Hz)", actualDftBw, mMaxDftBw );
            UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, FRA_WARNING );
        }
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
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, SAMPLE_PROCESSING_DIAGNOSTICS );

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
    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, SAMPLE_PROCESSING_DIAGNOSTICS );

    if (!(ps->GetPeakValues( inputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]], outputAbsMax[freqStepIndex][totalRetryCounter[freqStepIndex]], ovIn, ovOut )))
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
            inAmps[freqStepIndex][totalRetryCounter[freqStepIndex]] = inputAmplitude;
            outAmps[freqStepIndex][totalRetryCounter[freqStepIndex]] = outputAmplitude;
            inputPurity[freqStepIndex][totalRetryCounter[freqStepIndex]] = currentInputPurity;
            outputPurity[freqStepIndex][totalRetryCounter[freqStepIndex]] = currentOutputPurity;
        }

        if (mAdaptiveStimulus)
        {
            // Using the range indices recorded before auto-ranging, because auto-ranging could have changed the current index
            currentInputAmplitudeVolts = (inputAmplitude / (double)(ps->GetMaxValue())) *
                                         rangeInfo[adaptiveStimulusInputChannelRange].rangeVolts *
                                         attenInfo[mInputChannelAttenuation];
            currentOutputAmplitudeVolts = (outputAmplitude / (double)(ps->GetMaxValue())) *
                                          rangeInfo[adaptiveStimulusOutputChannelRange].rangeVolts *
                                          attenInfo[mOutputChannelAttenuation];

            swprintf( fraStatusText, 128, L"Status: Measured input amplitude: %0.3lf V", currentInputAmplitudeVolts );
            UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, ADAPTIVE_STIMULUS_DIAGNOSTICS );
            swprintf( fraStatusText, 128, L"Status: Measured output amplitude: %0.3lf V", currentOutputAmplitudeVolts );
            UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, ADAPTIVE_STIMULUS_DIAGNOSTICS );

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
        ps->GetCompressedData( compressedSize, inputMinData[freqStepIndex][totalRetryCounter[freqStepIndex]],
                                               outputMinData[freqStepIndex][totalRetryCounter[freqStepIndex]],
                                               inputMaxData[freqStepIndex][totalRetryCounter[freqStepIndex]],
                                               outputMaxData[freqStepIndex][totalRetryCounter[freqStepIndex]] );
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
        stimulusChanged = false;
        return true;
    }
    else
    {
        adaptiveStimulusRetryCounter += forceAdjust ? 0 : 1;
        // Bound the result.  Can't be higher than function generator maximum.  Need to avoid 0.0 or else future
        // adjustment will be bound to 0.0.  Don't update if it's our last allowed attempt.
        if (adaptiveStimulusRetryCounter < maxAdaptiveStimulusRetries)
        {
            currentStimulusVpp = max(ps->GetMinNonZeroFuncGenVpp(), min(mMaxStimulusVpp, max(newStimulusFromInput, newStimulusFromOutput)));
            stimulusChanged = true;
        }
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
// - The standard recurrence used by the original Goertzel is known to have numerical accuracy
//   issues - i.e. O(N^2) error growth for theta near 0 or PI.  This can start to be a real problem for scopes
//   with very large sample buffers (e.g. currently up to 1GS on the 6000 series).  The Reinsch modifications
//   are a well known technique to deal with error at these extremes.  As theta approaches PI/2, the Reinsch
//   recurrence numerical accuracy becomes worse than Goertzel (Oliver 77).  To achieve the least error, the
//   technique of Oliver will be used to divide the domain into three regions, applying the best recurrence
//   to each: (0.0 - 0.305*PI): REINSCH_0; (0.305*PI - 0.705*PI): GOERTZEL; (0.705*PI - PI): REINSCH_PI
//
// - The d.c. energy calculation can be seen as an execution of the Goertzel with Magic Circle Oscillator.
//   When the tuning frequency is 0Hz, the tuning parameter is 0.  Thus the energy calculation is simplified.
//    - The idea to use alternate oscillators comes from the work of Clay Turner on Oscillator theory and
//      application to the Goertzel:
//      Ref. http://www.claysturner.com/dsp/digital_resonators.pdf
//      Ref. http://www.claysturner.com/dsp/ResonatorTable.pdf
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
typedef enum
{
    REINSCH_0,
    GOERTZEL,
    REINSCH_PI
} DftRecurrence_T;

const double recurrenceThreshold1 = 0.305 * M_PI; // Oliver 77
const double recurrenceThreshold2 = 0.705 * M_PI; // Oliver 77
DftRecurrence_T dftRecurrence = REINSCH_0;
double theta;
alignas(16) static double Kappa;
alignas(16) static double a[2], b[2], τ[2];
alignas(16) static double totalEnergy[2];
alignas(16) static double dcEnergy[2];
static uint32_t samplesProcessed;
static uint32_t N;
// Goertzel outputs
static array<double,2> magnitude, phase, amplitude, purity;

void PicoScopeFRA::InitGoertzel( uint32_t totalSamples, double fSamp, double fDetect )
{
    double halfTheta;
    τ[0] = τ[1] = a[0] = a[1] = b[0] = b[1] = 0.0;
    totalEnergy[0] = totalEnergy[1] = 0.0;
    dcEnergy[0] = dcEnergy[1] = 0.0;
    samplesProcessed = 0;
    FRA_STATUS_MESSAGE_T fraStatusMsg;
    wchar_t fraStatusText[128];

    N = totalSamples;

    theta = 2.0 * M_PI * (fDetect / fSamp);

    if (theta < recurrenceThreshold1)
    {
        dftRecurrence = REINSCH_0;
        halfTheta = M_PI * (fDetect / fSamp);
        Kappa = -4.0 * pow( sin( halfTheta ), 2.0 );
        swprintf( fraStatusText, 128, L"Status: Computing DFT using Reinsch(0)" );
    }
    else if (theta < recurrenceThreshold2)
    {
        dftRecurrence = GOERTZEL;
        Kappa = 2.0 * cos( theta );
        swprintf( fraStatusText, 128, L"Status: Computing DFT using Goertzel" );
    }
    else
    {
        dftRecurrence = REINSCH_PI;
        halfTheta = M_PI * (fDetect / fSamp);
        Kappa = 4.0 * pow( cos( halfTheta ), 2.0 );
        swprintf( fraStatusText, 128, L"Status: Computing DFT using Reinsch(pi)" );
    }

    UpdateStatus( fraStatusMsg, FRA_STATUS_MESSAGE, fraStatusText, DFT_DIAGNOSTICS );
}

void PicoScopeFRA::FeedGoertzel( int16_t* inputSamples, int16_t* outputSamples, uint32_t n )
{
    bool lastBlock = false;

    // Vectors
    __m128d KappaVec, τVec, aVec, bVec, totalEnergyVec, dcEnergyVec, sampleVec;

    // Determine if this is the last block.  If it is, there is special processing.
    lastBlock = ((samplesProcessed + n) == N);

    // Load vectors
    KappaVec = _mm_load1_pd(&Kappa);
    τVec = _mm_load_pd(τ);
    aVec = _mm_load_pd(a);
    bVec = _mm_load_pd(b);
    totalEnergyVec = _mm_load_pd(totalEnergy);
    dcEnergyVec = _mm_load_pd(dcEnergy);

    // Execute the filter, d.c. Energy and Parseval energy time domain calculation
    if (REINSCH_0 == dftRecurrence)
    {
        for (uint32_t i = 0; i < n; i++)
        {
            // Load and convert samples to packed doubles
            sampleVec = _mm_cvtepi32_pd( _mm_set_epi32( 0, 0, outputSamples[i], inputSamples[i] ) );

            //totalEnergy += samples[i]*samples[i];
            totalEnergyVec = _mm_add_pd( totalEnergyVec, _mm_mul_pd( sampleVec, sampleVec ) );

            //dcEnergy += samples[i];
            dcEnergyVec = _mm_add_pd( dcEnergyVec, sampleVec );

            //b = b + K*a + samples[i]
            //a = a + b
            bVec = _mm_add_pd( _mm_add_pd(bVec, _mm_mul_pd(KappaVec, aVec)), sampleVec);
            aVec = _mm_add_pd(aVec, bVec);
        }
    }
    else if (GOERTZEL == dftRecurrence)
    {
        for (uint32_t i = 0; i < n; i++)
        {
            // Load and convert samples to packed doubles
            sampleVec = _mm_cvtepi32_pd( _mm_set_epi32( 0, 0, outputSamples[i], inputSamples[i] ) );

            //totalEnergy += samples[i]*samples[i];
            totalEnergyVec = _mm_add_pd( totalEnergyVec, _mm_mul_pd( sampleVec, sampleVec ) );

            //dcEnergy += samples[i];
            dcEnergyVec = _mm_add_pd( dcEnergyVec, sampleVec );

            //τ = K*a - b + samples[i];
            //b = a;
            //a = τ;
            τVec = _mm_add_pd( _mm_sub_pd( _mm_mul_pd( KappaVec, aVec ), bVec ), sampleVec );
            bVec = aVec;
            aVec = τVec;
        }
    }
    else // REINSCH_PI
    {
        for (uint32_t i = 0; i < n; i++)
        {
            // Load and convert samples to packed doubles
            sampleVec = _mm_cvtepi32_pd( _mm_set_epi32( 0, 0, outputSamples[i], inputSamples[i] ) );

            //totalEnergy += samples[i]*samples[i];
            totalEnergyVec = _mm_add_pd( totalEnergyVec, _mm_mul_pd( sampleVec, sampleVec ) );

            //dcEnergy += samples[i];
            dcEnergyVec = _mm_add_pd( dcEnergyVec, sampleVec );

            //b = K*a - b + samples[i]
            //a = b - a
            bVec = _mm_add_pd(_mm_sub_pd(_mm_mul_pd(KappaVec, aVec), bVec), sampleVec);
            aVec = _mm_sub_pd(bVec, aVec);
        }
    }

    samplesProcessed += n;

    if (lastBlock)
    {
        array<complex<double>, 2> y;
        array<double, 2> signalEnergy;

        // Iterate Goertzel once more with 0 input to get correct phase
        if (REINSCH_0 == dftRecurrence)
        {
            bVec = _mm_add_pd(bVec, _mm_mul_pd(KappaVec, aVec));
            aVec = _mm_add_pd(aVec, bVec);
        }
        else if (GOERTZEL == dftRecurrence)
        {
            τVec = _mm_sub_pd( _mm_mul_pd( KappaVec, aVec ), bVec );
            bVec = aVec;
            aVec = τVec;
        }
        else // REINSCH_PI
        {
            bVec = _mm_sub_pd(_mm_mul_pd(KappaVec, aVec), bVec);
            aVec = _mm_sub_pd(bVec, aVec);
        }

        // Unvectorize
        _mm_store_pd(a, aVec);
        _mm_store_pd(b, bVec);
        _mm_store_pd(totalEnergy, totalEnergyVec);
        _mm_store_pd(dcEnergy, dcEnergyVec);

        // Compute the complex output
        if (REINSCH_0 == dftRecurrence)
        {
            y[0] = std::complex<double>(b[0] + a[0] * Kappa/2.0, a[0] * sin(theta));
            y[1] = std::complex<double>(b[1] + a[1] * Kappa/2.0, a[1] * sin(theta));
        }
        else if (GOERTZEL == dftRecurrence)
        {
            y[0] = std::complex<double>(a[0] - b[0] * cos(theta), b[0] * sin(theta));
            y[1] = std::complex<double>(a[1] - b[1] * cos(theta), b[1] * sin(theta));
        }
        else // REINSCH_PI
        {
            y[0] = std::complex<double>(b[0] - a[0] * Kappa/2.0, -a[0] * sin(theta));
            y[1] = std::complex<double>(b[1] - a[1] * Kappa/2.0, -a[1] * sin(theta));
        }

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
        _mm_store_pd(τ, τVec);
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
