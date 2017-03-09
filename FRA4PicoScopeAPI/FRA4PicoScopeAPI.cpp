//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frequency Response Analyzer for PicoScope
//
// Copyright (c) 2016 by Aaron Hexamer
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
// Module FRA4PicoScopeAPI.cpp: Implementation for the FRA4PicoScope API DLL.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FRA4PicoScopeAPI.h"
#include "ScopeSelector.h"
#include "PicoScopeFRA.h"

bool bInitialized = false;
ScopeSelector* pScopeSelector = NULL;
PicoScopeFRA* pFRA = NULL;
FRA_STATUS_T status = FRA_STATUS_IDLE;

HANDLE hExecuteFraThread;
HANDLE hExecuteFraEvent;

wstring messageLog;
bool bLogMessages = false;
uint16_t logVerbosityFlags = SCOPE_ACCESS_DIAGNOSTICS | FRA_PROGRESS | STEP_TRIAL_PROGRESS |
                             SIGNAL_GENERATOR_DIAGNOSTICS | AUTORANGE_DIAGNOSTICS |
                             ADAPTIVE_STIMULUS_DIAGNOSTICS | SAMPLE_PROCESSING_DIAGNOSTICS |
                             SCOPE_POWER_EVENTS | DFT_DIAGNOSTICS | FRA_WARNING;
bool bAutoClearLog = true;
static const size_t messageLogSizeLimit = 16777216; // 16MB
FRA_STATUS_CALLBACK FraStatusCallback = NULL;

static DWORD WINAPI ExecuteFRA(LPVOID lpdwThreadParam);
static bool LocalFraStatusCallback( FRA_STATUS_MESSAGE_T& fraStatus );
void LogMessage(const wstring statusMessage, LOG_MESSAGE_FLAGS_T type = FRA_ERROR );

// Storage for default/initial FRA parameters
// For basic settings
static const SamplingMode_T samplingModeDefault = LOW_NOISE;
static const bool adaptiveStimulusModeDefault = false;
static const double targetResponseAmplitudeDefault = 0.0;
static const bool sweepDescendingDefault = false;
static const double phaseWrappingThresholdDefault = 180.0;
// For tuning
static const double purityLowerLimitDefault = 0.80;
static const uint16_t extraSettlingTimeMsDefault = 0;
static const uint8_t autorangeTriesPerStepDefault = 10;
static const double autorangeToleranceDefault = 0.10;
static const double smallSignalResolutionToleranceDefault = 0.0;
static const double maxAutorangeAmplitudeDefault = 1.0;
static const int32_t inputStartRangeDefault = -1;
static const int32_t outputStartRangeDefault = 0;
static const uint8_t adaptiveStimulusTriesPerStepDefault = 10;
static const double targetResponseAmplitudeToleranceDefault = 0.1; // 10%
static const uint16_t minCyclesCapturedDefault = 16;
static const double maxDftBwDefault = 100.0;
static const uint16_t lowNoiseOversamplingDefault = 64;

// Parameters used to communicate from API to execution thread
static double startFreqHz = 0.0;
static double stopFreqHz = 0.0;
static int stepsPerDecade = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SetCallback
//
// Purpose: Install a function to be called back when FRA status updates occur
//
// Parameters: [in] fraCb - pointer to the callback function
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void __stdcall SetCallback( FRA_STATUS_CALLBACK fraCb )
{
    FraStatusCallback = fraCb;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Initialize
//
// Purpose: Initialize data/objects, event handles, threads, etc.
//
// Parameters: [out] - returns a status indicating whether the initialization was successful
//
// Notes: Not called by DllMain to follow good design practice
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool __stdcall Initialize( void )
{
    if (!bInitialized)
    {
        pScopeSelector = new ScopeSelector();
        pFRA = new PicoScopeFRA(LocalFraStatusCallback);

        if (pScopeSelector && pFRA)
        {
            pFRA->SetFraSettings( samplingModeDefault, adaptiveStimulusModeDefault, targetResponseAmplitudeDefault,
                                  sweepDescendingDefault, phaseWrappingThresholdDefault );

            pFRA->SetFraTuning( purityLowerLimitDefault, extraSettlingTimeMsDefault, autorangeTriesPerStepDefault, autorangeToleranceDefault,
                                smallSignalResolutionToleranceDefault, maxAutorangeAmplitudeDefault, inputStartRangeDefault, inputStartRangeDefault,
                                adaptiveStimulusTriesPerStepDefault, targetResponseAmplitudeToleranceDefault, minCyclesCapturedDefault, maxDftBwDefault,
                                lowNoiseOversamplingDefault );

            pFRA->DisableDiagnostics();

            // Create execution thread and event
            hExecuteFraEvent = CreateEventW(NULL, true, false, L"ExecuteFRA");

            if ((HANDLE)NULL == hExecuteFraEvent)
            {
                LogMessage(L"Error: Could not initialize application event \"ExecuteFRA\".");
            }
            else if (ERROR_ALREADY_EXISTS == GetLastError())
            {
                LogMessage(L"Error: Cannot run multiple instances of FRA4PicoScopeAPI.");
            }
            else if (!ResetEvent(hExecuteFraEvent))
            {
                LogMessage(L"Error: Could not reset application event \"ExecuteFRA\".");
            }
            else
            {
                hExecuteFraThread = CreateThread(NULL, 0, ExecuteFRA, NULL, 0, NULL);
                if ((HANDLE)NULL == hExecuteFraThread)
                {
                    LogMessage(L"Error: Could not initialize application FRA execution thread.");
                }
                else
                {
                    bInitialized = true;
                }
            }
        }
    }

    return bInitialized;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Cleanup
//
// Purpose: De-initialize data/objects, event handles, threads, etc.
//
// Parameters: N/A
//
// Notes: Not called by DllMain to follow good design practice
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void __stdcall Cleanup( void )
{
    delete pScopeSelector;
    delete pFRA;

    pScopeSelector = NULL;
    pFRA = NULL;

    CloseHandle(hExecuteFraEvent);
    CloseHandle(hExecuteFraThread);

    bInitialized = false;
}

bool GetLogVerbosityFlag(LOG_MESSAGE_FLAGS_T flag)
{
    return (((LOG_MESSAGE_FLAGS_T)logVerbosityFlags & flag) == flag);
}

void __stdcall SetLogVerbosityFlag(LOG_MESSAGE_FLAGS_T flag, bool set)
{
    if (set)
    {
        logVerbosityFlags  = (logVerbosityFlags | (uint16_t)flag);
    }
    else
    {
        logVerbosityFlags  = (logVerbosityFlags & ~(uint16_t)flag);
    }
}

void __stdcall SetLogVerbosityFlags(LOG_MESSAGE_FLAGS_T flags)
{
    logVerbosityFlags = (uint16_t)flags;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: LogMessage
//
// Purpose: Logs messages to the internal log, which can be retrieved later by the client
//
// Parameters: [in] statusMessage - the text to be logged
//             [in] type - type of message, used to determine whether to log a message based on
//                         verbosity settings
//
// Notes: Limits message log to messageLogSizeLimit.  If it would exceed this it will be cleared
//        first before adding the new message.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void LogMessage( const wstring statusMessage, LOG_MESSAGE_FLAGS_T type )
{
    if (bLogMessages && (type == FRA_ERROR || GetLogVerbosityFlag(type)))
    {
        if (messageLog.size() + statusMessage.size() > messageLogSizeLimit)
        {
            messageLog = TEXT("");
        }
        messageLog += statusMessage + TEXT("\n");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SetScope
//
// Purpose: Open the scope and set it for FRA use
//
// Parameters: [in] sn - the serial number of the desired scope
//             [out] - returns a status indicating whether the operation succeeded
//
// Notes: If the string is null or empty, it will try to open the only scope attached
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool __stdcall SetScope( char* sn )
{
    bool retVal = false;
    PicoScope* pScope;
    bool bScopeFound = false;
    std::vector<AvailableScopeDescription_T> scopes;
    AvailableScopeDescription_T scopeToOpen;
    uint8_t idx;

    if (bInitialized)
    {
        pScopeSelector->GetAvailableScopes(scopes);

        if ((NULL == sn || sn[0] == 0) && scopes.size() == 1)
        {
            bScopeFound = true;
            scopeToOpen = scopes[0];
        }
        else
        {
            for (idx = 0; idx < scopes.size(); idx++)
            {
                if (scopes[idx].serialNumber == sn)
                {
                    bScopeFound = true;
                    scopeToOpen = scopes[idx];
                    break;
                }
            }
        }

        if (bScopeFound)
        {
            pScope = pScopeSelector->OpenScope(scopeToOpen);
            if (pScope)
            {
                pFRA->SetInstrument(pScope);
                retVal = true;
            }
        }
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: GetMinFrequency
//
// Purpose: Returns the minimum frequency setting possible.  I.e. the minimum possible value for
//          the first parameter to StartFRA
//
// Parameters: [out] - returns the minimum frequency in Hz
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

double __stdcall GetMinFrequency( void )
{
    return pFRA->GetMinFrequency();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: StartFRA
//
// Purpose: Starts the FRA and immediately returns allowing the caller to do other processing
//
// Parameters: [in] _startFreqHz - Beginning frequency
//             [in] _stopFreqHz - End frequency
//             [in] _stepsPerDecade - Samples per every log10 frequency
//             [out] - returns a status indicating whether the operation succeeded
//
// Notes: Since it returns immediately, the caller can either poll for completion, or receive
//        callbacks indicating status
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool __stdcall StartFRA( double _startFreqHz, double _stopFreqHz, int _stepsPerDecade )
{
    bool retVal = false;

    if (bInitialized)
    {
        startFreqHz = _startFreqHz;
        stopFreqHz = _stopFreqHz;
        stepsPerDecade = _stepsPerDecade;
        if (WaitForSingleObject(hExecuteFraEvent, 0) == WAIT_TIMEOUT) // Is the event not already signalled?
        {
            retVal = SetEvent(hExecuteFraEvent) ? true : false;
            if (retVal)
            {
                status = FRA_STATUS_IN_PROGRESS;
            }
        }
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: CancelFRA
//
// Purpose: Stops the FRA at the earliest possible stopping point
//
// Parameters: [out] - returns a status indicating whether the operation succeeded
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool __stdcall CancelFRA( void )
{
    bool retVal = false;

    if (bInitialized)
    {
        retVal = pFRA->CancelFRA();
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: GetFraStatus
//
// Purpose: Gets the status of the FRA
//
// Parameters: [out] - returns a status indicating FRA progress, completion or error
//
// Notes: Because this is for polled mode, some possible values of FRA_STATUS_T won't be returned.
//        E.g. status messages won't be seen because the actual text is only available in the
//        callback interface.  However the messages can still be retrieved through the message
//        log even while in polled mode.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

FRA_STATUS_T __stdcall GetFraStatus( void )
{
    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SetFraSettings
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

void __stdcall SetFraSettings( SamplingMode_T samplingMode, bool adaptiveStimulusMode, double targetResponseAmplitude,
                               bool sweepDescending, double phaseWrappingThreshold )
{
    if (pFRA)
    {
        pFRA->SetFraSettings( samplingMode, adaptiveStimulusMode, targetResponseAmplitude,
                              sweepDescending, phaseWrappingThreshold );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SetFraTuning
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

void __stdcall SetFraTuning( double purityLowerLimit, uint16_t extraSettlingTimeMs, uint8_t autorangeTriesPerStep,
                             double autorangeTolerance, double smallSignalResolutionTolerance, double maxAutorangeAmplitude,
                             int32_t inputStartRange, int32_t outputStartRange, uint8_t adaptiveStimulusTriesPerStep,
                             double targetResponseAmplitudeTolerance, uint16_t minCyclesCaptured, double maxDftBw,
                             uint16_t lowNoiseOversampling )
{
    if (pFRA)
    {
        pFRA->SetFraTuning( purityLowerLimit, extraSettlingTimeMs, autorangeTriesPerStep, autorangeTolerance,
                            smallSignalResolutionTolerance, maxAutorangeAmplitude, inputStartRange, outputStartRange,
                            adaptiveStimulusTriesPerStep, targetResponseAmplitudeTolerance, minCyclesCaptured, maxDftBw,
                            lowNoiseOversampling );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SetupChannels
//
// Purpose: Set the channel settings used in the FRA
//
// Parameters: [in] inputChannel - Channel to use for input signal
//             [in] inputChannelCoupling - AC/DC coupling for input channel
//             [in] inputChannelAttenuation - Attenuation setting for input channel
//             [in] inputDcOffset - DC Offset for input channel
//             [in] outputChannel - Channel to use for output signal
//             [in] outputChannelCoupling - AC/DC coupling for output channel
//             [in] outputChannelAttenuation - Attenuation setting for output channel
//             [in] outputDcOffset - DC Offset for output channel
//             [in] initialStimulusVpp - Volts peak to peak of the stimulus signal
//             [in] maxStimulusVpp - Maximum volts peak to peak of the stimulus signal
//             [out] return - Whether the function was successful.
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool __stdcall SetupChannels( int inputChannel, int inputChannelCoupling, int inputChannelAttenuation, double inputDcOffset,
                              int outputChannel, int outputChannelCoupling, int outputChannelAttenuation, double outputDcOffset,
                              double initialStimulusVpp, double maxStimulusVpp )
{
    if (pFRA)
    {
        return (pFRA->SetupChannels( inputChannel, inputChannelCoupling, inputChannelAttenuation, inputDcOffset,
                                     outputChannel, outputChannelCoupling, outputChannelAttenuation, outputDcOffset,
                                     initialStimulusVpp, maxStimulusVpp ));
    }
    else
    {
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: GetNumSteps
//
// Purpose: Called after FRA execution completes to get the number of frequency samples taken
//          Generally used to know how large the results arrays passed to GetResults need to be
//
// Parameters: [out] - the number of steps (frequency samples)
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

int __stdcall GetNumSteps( void )
{
    int retVal = 0;
    double *freqsLogHz, *gainsDb, *phasesDeg, *unwrappedPhasesDeg;

    if (pFRA)
    {
        pFRA->GetResults( &retVal, &freqsLogHz, &gainsDb, &phasesDeg, &unwrappedPhasesDeg );
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: GetResults
//
// Purpose: Gets the FRA results
//
// Parameters: [out] freqsLogHz - array of frequency points taken, expressed in log base 10 Hz
//             [out] gainsDb - array of gains at each frequency point of freqsLogHz, expressed in dB
//             [out] phasesDeg - array of phase shifts at each frequency point of freqsLogHz, expressed in degrees
//             [out] unwrappedPhasesDeg - array of unwrapped phase shifts at each frequency point of freqsLogHz,
//                                        expressed in degrees
//
// Notes: Arrays are owned and to be properly allocted by the caller.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void __stdcall GetResults( double* freqsLogHz, double* gainsDb, double* phasesDeg, double* unwrappedPhasesDeg )
{
    int numSteps;
    double *_freqsLogHz = NULL, *_gainsDb = NULL, *_phasesDeg = NULL, *_unwrappedPhasesDeg = NULL;

    if (pFRA)
    {
        pFRA->GetResults(&numSteps, &_freqsLogHz, &_gainsDb, &_phasesDeg, &_unwrappedPhasesDeg);

        if (freqsLogHz && _freqsLogHz)
        {
            memcpy(freqsLogHz, _freqsLogHz, numSteps*sizeof(double));
        }
        if (gainsDb && _gainsDb)
        {
            memcpy(gainsDb, _gainsDb, numSteps*sizeof(double));
        }
        if (phasesDeg && _phasesDeg)
        {
            memcpy(phasesDeg, _phasesDeg, numSteps*sizeof(double));
        }
        if (unwrappedPhasesDeg && _unwrappedPhasesDeg)
        {
            memcpy(unwrappedPhasesDeg, _unwrappedPhasesDeg, numSteps*sizeof(double));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: EnableDiagnostics
//
// Purpose: Turn on time domain diagnostic plot output
//
// Parameters: [in] baseDataPath - where to put the "diag" directory, where the plot files will
//                                 be stored
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void __stdcall EnableDiagnostics( wchar_t* baseDataPath )
{
    if (pFRA)
    {
        pFRA -> EnableDiagnostics( baseDataPath );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: DisableDiagnostics
//
// Purpose: Turn off time domain diagnostic plot output
//
// Parameters: N/A
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void __stdcall DisableDiagnostics( void )
{
    if (pFRA)
    {
        pFRA -> DisableDiagnostics();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: AutoClearMessageLog
//
// Purpose: Turn on or off auto-clearing of the message log.  If on, the message log gets cleared
//          at the start of each execution of FRA
//
// Parameters: [in] autoClear - true if on, false if off
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void __stdcall AutoClearMessageLog( bool bAutoClear )
{
    bAutoClearLog = bAutoClear;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: EnableMessageLog
//
// Purpose: Turn on or off message logging
//
// Parameters: [in] bEnable - true if on, false if off
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void __stdcall EnableMessageLog( bool bEnable )
{
    bLogMessages = bEnable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: GetMessageLog
//
// Purpose: Gets a reference to the message log
//
// Parameters: [out] returns a pointer to a wide character string which contains the message log.
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

const wchar_t* __stdcall GetMessageLog( void )
{
    return messageLog.c_str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ClearMessageLog
//
// Purpose: Resets the message log
//
// Parameters: N/A
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void __stdcall ClearMessageLog( void )
{
    messageLog = TEXT("");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ExecuteFRA
//
// Purpose: Internal thread function that controls the execution of the FRA and status setting.
//
// Parameters: See Windows API documentation
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI ExecuteFRA( LPVOID lpdwThreadParam )
{
    DWORD dwWaitResult;

    for (;;)
    {
        if (!ResetEvent(hExecuteFraEvent))
        {
            LogMessage(L"Fatal error: Failed to reset FRA execution start event");
            status = FRA_STATUS_FATAL_ERROR;
            return -1;
        }

        dwWaitResult = WaitForSingleObject(hExecuteFraEvent, INFINITE);

        if (dwWaitResult == WAIT_OBJECT_0)
        {
            if (bAutoClearLog)
            {
                ClearMessageLog();
            }

            if (NULL == pScopeSelector->GetSelectedScope()) // Scope not created
            {
                LogMessage(L"Error: Device not initialized.");
                status = FRA_STATUS_FATAL_ERROR;
                continue;
            }
            else if (!(pScopeSelector->GetSelectedScope()->IsCompatible()))
            {
                LogMessage(L"Error: Selected scope is not compatible.");
                status = FRA_STATUS_FATAL_ERROR;
                continue;
            }

            if (false == pFRA->ExecuteFRA(startFreqHz, stopFreqHz, stepsPerDecade))
            {
                continue;
                status = FRA_STATUS_FATAL_ERROR;
            }
        }
        else
        {
            LogMessage(L"Fatal error: Invalid result from waiting on FRA execution start event");
            status = FRA_STATUS_FATAL_ERROR;
            return -1;
        }
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: LocalFraStatusCallback
//
// Purpose: The actual callback passed to the PicoScopeFRA object.  It takes care of status setting
//          for the polled case, calls the DLL client's callback (if it was set), and logs messages
//          in the local message log.
//
// Parameters: [in] fraStatus - the status object passed by the PicoScopeFRA object.
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

static bool LocalFraStatusCallback( FRA_STATUS_MESSAGE_T& fraStatus )
{
    if (FraStatusCallback)
    {
        status = fraStatus.status;
        FraStatusCallback( fraStatus );
    }
    else
    {
        // Don't pass along message status in polled mode since they are not
        // a terminal state and happen while FRA is in-progress
        if (FRA_STATUS_MESSAGE != fraStatus.status)
        {
            status = fraStatus.status;
        }
        // Set interactive response parameters in a way to indicate that we should 
        // not proceed because there was no way to gather response from the application level.
        if (FRA_STATUS_RETRY_LIMIT == fraStatus.status ||
            FRA_STATUS_POWER_CHANGED == fraStatus.status)
        {
            fraStatus.responseData.proceed = false;
        }
    }

    if (FRA_STATUS_FATAL_ERROR == fraStatus.status)
    {
        LogMessage(fraStatus.statusText);
    }
    else if (FRA_STATUS_MESSAGE == fraStatus.status)
    {
        LogMessage(fraStatus.statusText, fraStatus.messageType);
    }

    return true;
}
