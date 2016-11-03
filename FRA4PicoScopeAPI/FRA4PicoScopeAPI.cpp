// FRA4PicoScopeAPI.cpp : Defines the implementation of the FRA4PicoScope API DLL.
//

#include "stdafx.h"
#include "FRA4PicoScopeAPI.h"
#include "ScopeSelector.h"
#include "PicoScopeFRA.h"

bool bInitialized = false;
ScopeSelector* pScopeSelector = NULL;
PicoScopeFRA* pFRA = NULL;

HANDLE hExecuteFraThread;
HANDLE hExecuteFraEvent;

wstring messageLog;

DWORD WINAPI ExecuteFRA(LPVOID lpdwThreadParam);
bool FraStatusCallback( FRA_STATUS_MESSAGE_T& fraStatus );

bool Initialize( void )
{
    if (!bInitialized)
    {
        pScopeSelector = new ScopeSelector();
        pFRA = new PicoScopeFRA(FraStatusCallback); // TBD callback
        
        if (pScopeSelector && pFRA)
        {
            bInitialized = true;
        }
    }

    // Create execution thread and event
    hExecuteFraEvent = CreateEventW(NULL, true, false, L"ExecuteFRA");

    if ((HANDLE)NULL == hExecuteFraEvent)
    {
        MessageBox(0, L"Could not initialize application event \"ExecuteFRA\".", L"Fatal Error", MB_OK);
    }
    else if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        MessageBox(0, L"Cannot run multiple instances of FRA4PicoScope.", L"Fatal Error", MB_OK);
    }

    if (!ResetEvent(hExecuteFraEvent))
    {
        MessageBox(0, L"Could not reset application event \"ExecuteFRA\".", L"Fatal Error", MB_OK);
    }

    hExecuteFraThread = CreateThread(NULL, 0, ExecuteFRA, NULL, 0, NULL);

    if ((HANDLE)NULL == hExecuteFraThread)
    {
        MessageBox(0, L"Could not initialize application FRA execution thread.", L"Fatal Error", MB_OK);
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
    return 0.0;
}

bool StartFRA( double startFreqHz, double stopFreqHz, int stepsPerDecade )
{
    return true;
}

bool CancelFRA( void )
{
    return true;
}

FRA4PICOSCOPE_API FraApiStatus_T GetFraStatus(void)
{
    return FRA_API_STATUS_COMPLETE;
}

void SetFraSettings( SamplingMode_T samplingMode, bool sweepDescending, double phaseWrappingThreshold )
{

}

void SetFraTuning( double purityLowerLimit, uint16_t extraSettlingTimeMs, uint8_t autorangeTriesPerStep,
                   double autorangeTolerance, double smallSignalResolutionTolerance, double maxAutorangeAmplitude, uint16_t minCyclesCaptured )
{

}

bool SetupChannels( int inputChannel, int inputChannelCoupling, int inputChannelAttenuation, double inputDcOffset,
                    int outputChannel, int outputChannelCoupling, int outputChannelAttenuation, double outputDcOffset,
                    double signalVpp)
{
    return true;
}

void GetResults( int* numSteps, double** freqsLogHz, double** gainsDb, double** phasesDeg, double** unwrappedPhasesDeg )
{

}

void EnableDiagnostics( wchar_t* baseDataPath )
{

}

void DisableDiagnostics( void )
{

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
    return 0;
}

bool FraStatusCallback( FRA_STATUS_MESSAGE_T& fraStatus )
{
    return true;
}
