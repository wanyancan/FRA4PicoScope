// FRA4PicoScopeAPI.cpp : Defines the implementation of the FRA4PicoScope API DLL.
//

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
bool bAutoClearLog = true;
FRA_STATUS_CALLBACK FraStatusCallback = NULL;

DWORD WINAPI ExecuteFRA(LPVOID lpdwThreadParam);
bool LocalFraStatusCallback( FRA_STATUS_MESSAGE_T& fraStatus );
void LogMessage(const wstring statusMessage);

// Storage for passed FRA parameters
static int inputChannel = 0;
static int inputChannelCoupling = 0;
static int inputChannelAttenuation = 0;
static double inputDcOffset = 0.0;
static int outputChannel = 0;
static int outputChannelCoupling = 0;
static int outputChannelAttenuation = 0;
static double outputDcOffset = 0.0;
static double signalVpp = 0.0;
static double startFreqHz = 0.0;
static double stopFreqHz = 0.0;
static int stepsPerDecade = 0;
SamplingMode_T samplingMode = LOW_NOISE;
bool sweepDescending = false;
double phaseWrappingThreshold = 180.0;
static double purityLowerLimit = 0.80; // Default
static uint16_t extraSettlingTimeMs = 0; // Default
static uint8_t autorangeTriesPerStep = 10; // Default
static double autorangeTolerance = 0.10; // Default
static double smallSignalResolutionTolerance = 0.0; // Default
static double maxAutorangeAmplitude = 1.0; // Default
static uint16_t minCyclesCaptured = 16; // Default
static bool diagnosticsOn = false; // Default
static wstring baseDataPath = L"";

void __stdcall SetCallback( FRA_STATUS_CALLBACK fraCb )
{
    FraStatusCallback = fraCb;
}

bool __stdcall Initialize( void )
{
    if (!bInitialized)
    {
        pScopeSelector = new ScopeSelector();
        pFRA = new PicoScopeFRA(LocalFraStatusCallback);

        if (pScopeSelector && pFRA)
        {
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

void LogMessage( const wstring statusMessage )
{
    if (bLogMessages)
    {
        messageLog += statusMessage + TEXT("\n");
    }
}

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

double __stdcall GetMinFrequency( void )
{
    return pFRA->GetMinFrequency();
}

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

bool __stdcall CancelFRA( void )
{
    bool retVal = false;

    if (bInitialized)
    {
        retVal = pFRA->CancelFRA();
    }

    return retVal;
}

FRA_STATUS_T __stdcall GetFraStatus( void )
{
    return status;
}

void __stdcall SetFraSettings( SamplingMode_T _samplingMode, bool _sweepDescending, double _phaseWrappingThreshold )
{
    samplingMode = _samplingMode;
    sweepDescending = _sweepDescending;
    phaseWrappingThreshold = _phaseWrappingThreshold;
}

void __stdcall SetFraTuning( double _purityLowerLimit, uint16_t _extraSettlingTimeMs, uint8_t _autorangeTriesPerStep,
                             double _autorangeTolerance, double _smallSignalResolutionTolerance, double _maxAutorangeAmplitude, uint16_t _minCyclesCaptured )
{
    purityLowerLimit = _purityLowerLimit;
    extraSettlingTimeMs = _extraSettlingTimeMs;
    autorangeTriesPerStep = _autorangeTriesPerStep;
    autorangeTolerance = _autorangeTolerance;
    smallSignalResolutionTolerance = _smallSignalResolutionTolerance;
    maxAutorangeAmplitude = _maxAutorangeAmplitude;
    minCyclesCaptured = _minCyclesCaptured;
}

bool __stdcall SetupChannels( int _inputChannel, int _inputChannelCoupling, int _inputChannelAttenuation, double _inputDcOffset,
                              int _outputChannel, int _outputChannelCoupling, int _outputChannelAttenuation, double _outputDcOffset,
                              double _signalVpp)
{
    inputChannel = _inputChannel;
    inputChannelCoupling = _inputChannelCoupling;
    inputChannelAttenuation = _inputChannelAttenuation;
    inputDcOffset = _inputDcOffset;
    outputChannel = _outputChannel;
    outputChannelCoupling = _outputChannelCoupling;
    outputChannelAttenuation = _outputChannelAttenuation;
    outputDcOffset = _outputDcOffset;
    signalVpp = _signalVpp;

    return true;
}

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

void __stdcall EnableDiagnostics( wchar_t* _baseDataPath )
{
    baseDataPath = _baseDataPath;
    diagnosticsOn = true;
}

void __stdcall DisableDiagnostics( void )
{
    diagnosticsOn = false;
}

void __stdcall AutoClearMessageLog( bool bAutoClear )
{
    bAutoClearLog = bAutoClear;
}

void __stdcall EnableMessageLog( bool bEnable )
{
    bLogMessages = bEnable;
}

const wchar_t* __stdcall GetMessageLog( void )
{
    return messageLog.c_str();
}

void __stdcall ClearMessageLog( void )
{
    messageLog = TEXT("");
}

DWORD WINAPI ExecuteFRA( LPVOID lpdwThreadParam )
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

            pFRA->SetFraSettings( samplingMode, purityLowerLimit, extraSettlingTimeMs,
                                  autorangeTriesPerStep, autorangeTolerance, smallSignalResolutionTolerance,
                                  maxAutorangeAmplitude, minCyclesCaptured, sweepDescending,
                                  phaseWrappingThreshold, diagnosticsOn, baseDataPath );

            if (false == pFRA->SetupChannels(inputChannel, inputChannelCoupling, inputChannelAttenuation, inputDcOffset,
                                             outputChannel, outputChannelCoupling, outputChannelAttenuation, outputDcOffset,
                                             signalVpp))
            {
                continue;
                status = FRA_STATUS_FATAL_ERROR;
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

bool LocalFraStatusCallback( FRA_STATUS_MESSAGE_T& fraStatus )
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
        if (FRA_STATUS_AUTORANGE_LIMIT == fraStatus.status ||
            FRA_STATUS_POWER_CHANGED == fraStatus.status)
        {
            fraStatus.responseData.proceed = false;
        }
    }

    if (FRA_STATUS_MESSAGE == fraStatus.status)
    {
        LogMessage(fraStatus.statusText);
    }

    return true;
}
