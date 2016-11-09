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

bool Initialize( FRA_STATUS_CALLBACK fraCb )
{
    if (!bInitialized)
    {
        pScopeSelector = new ScopeSelector();
        pFRA = new PicoScopeFRA(LocalFraStatusCallback);
        FraStatusCallback = fraCb;

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

void Cleanup( void )
{
    delete pScopeSelector;
    delete pFRA;

    pScopeSelector = NULL;
    pFRA = NULL;

    bInitialized = false;
}

void LogMessage( const wstring statusMessage )
{
    messageLog += statusMessage;
}

bool SetScope( char* sn )
{
    return true;
}

double GetMinFrequency( void )
{
    return pFRA->GetMinFrequency();
}

bool StartFRA( double _startFreqHz, double _stopFreqHz, int _stepsPerDecade )
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
        }
    }

    return retVal;
}

bool CancelFRA( void )
{
    return pFRA->CancelFRA();
}

FRA4PICOSCOPE_API FRA_STATUS_T GetFraStatus(void)
{
    return status;
}

void SetFraSettings( SamplingMode_T _samplingMode, bool _sweepDescending, double _phaseWrappingThreshold )
{
    samplingMode = _samplingMode;
    sweepDescending = _sweepDescending;
    phaseWrappingThreshold = _phaseWrappingThreshold;
}

void SetFraTuning( double _purityLowerLimit, uint16_t _extraSettlingTimeMs, uint8_t _autorangeTriesPerStep,
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

bool SetupChannels( int _inputChannel, int _inputChannelCoupling, int _inputChannelAttenuation, double _inputDcOffset,
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

void GetResults( int* numSteps, double** freqsLogHz, double** gainsDb, double** phasesDeg, double** unwrappedPhasesDeg )
{

}

void EnableDiagnostics( wchar_t* _baseDataPath )
{
    baseDataPath = _baseDataPath;
    diagnosticsOn = true;
}

void DisableDiagnostics( void )
{
    diagnosticsOn = false;
}

const wchar_t* GetMessageLog(void)
{
    return messageLog.c_str();
}

void ClearMessageLog(void)
{
    messageLog = TEXT("");
}

DWORD WINAPI ExecuteFRA(LPVOID lpdwThreadParam)
{
    DWORD dwWaitResult;

    for (;;)
    {
        if (!ResetEvent(hExecuteFraEvent))
        {
            LogMessage(L"Fatal error: Failed to reset FRA execution start event");
            return -1;
        }

        dwWaitResult = WaitForSingleObject(hExecuteFraEvent, INFINITE);

        if (dwWaitResult == WAIT_OBJECT_0)
        {
            if (NULL == pScopeSelector->GetSelectedScope()) // Scope not created
            {
                LogMessage(L"Error: Device not initialized.");
                continue;
            }
            else if (!(pScopeSelector->GetSelectedScope()->IsCompatible()))
            {
                LogMessage(L"Error: Selected scope is not compatible.");
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
            }

            if (false == pFRA->ExecuteFRA(startFreqHz, stopFreqHz, stepsPerDecade))
            {
                continue;
            }
        }
        else
        {
            LogMessage(L"Fatal error: Invalid result from waiting on FRA execution start event");
            return -1;
        }
    }

    return TRUE;
}

bool LocalFraStatusCallback( FRA_STATUS_MESSAGE_T& fraStatus )
{
    status = fraStatus.status;

    if (FraStatusCallback)
    {
        FraStatusCallback( fraStatus );
    }
    else
    {
        // Set interactive response parameters in a way to indicate that we should 
        // not proceed because there was no way to gather response from the application level.
        if (FRA_STATUS_AUTORANGE_LIMIT == fraStatus.status ||
            FRA_STATUS_POWER_CHANGED == fraStatus.status)
        {
            fraStatus.responseData.proceed = false;
        }
    }

    return true;
}
