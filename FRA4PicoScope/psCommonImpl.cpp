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
// Module: psCommonImpl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <sstream>
#include <algorithm>

// Define lower case and upper case tokens
#if defined(PS2000)
#define SCOPE_FAMILY_LT 2000
#define SCOPE_FAMILY_UT 2000
#elif defined(PS3000)
#define SCOPE_FAMILY_LT 3000
#define SCOPE_FAMILY_UT 3000
#elif defined(PS2000A)
#define SCOPE_FAMILY_LT 2000a
#define SCOPE_FAMILY_UT 2000A
#define NEW_PS_DRIVER_MODEL
#elif defined(PS3000A)
#define SCOPE_FAMILY_LT 3000a
#define SCOPE_FAMILY_UT 3000A
#define NEW_PS_DRIVER_MODEL
#elif defined(PS4000)
#define SCOPE_FAMILY_LT 4000
#define SCOPE_FAMILY_UT 4000
#define NEW_PS_DRIVER_MODEL
#elif defined(PS5000A)
#define SCOPE_FAMILY_LT 5000a
#define SCOPE_FAMILY_UT 5000A
#define NEW_PS_DRIVER_MODEL
#endif

#if defined(NEW_PS_DRIVER_MODEL)
#define PICO_ERROR(x) PICO_OK != (status = x)
#else
#define PICO_ERROR(x) 0 == (status = x)
#define GetUnitInfo _get_unit_info
#define SetChannel _set_channel
#define SetSigGenBuiltIn _set_sig_gen_built_in
#define GetTimebase _get_timebase
#define SetTriggerChannelConditions _set_trigger
#define CloseUnit _close_unit
#endif

#define CommonClass(FM) xCommonClass(FM)
#define xCommonClass(FM) ps##FM##Impl

#define CommonCtor(FM) xCommonCtor(FM)
#define xCommonCtor(FM) ps##FM##Impl::ps##FM##Impl

#define CommonDtor(FM) xCommonDtor(FM)
#define xCommonDtor(FM) ps##FM##Impl::~ps##FM##Impl

#define CommonMethod(FM, METHOD) xCommonMethod(FM, METHOD)
#define CommonApi(FM, API) xCommonApi(FM, API)
#define xCommonMethod(FM, METHOD) ps##FM##Impl::##METHOD
#define xCommonApi(FM, API) ps##FM##API

#define CommonEnum(FM, ENUM) xCommonEnum(FM, ENUM)
#define xCommonEnum(FM, ENUM) PS##FM##_##ENUM

#define CommonSine(FM) xCommonSine(FM)
#define xCommonSine(FM) PS##FM##_##SINE

#define CommonEsOff(FM) xCommonEsOff(FM)
#define xCommonEsOff(FM) PS##FM##_##ES_OFF

#define CommonSigGenNone(FM) xCommonSigGenNone(FM)
#define xCommonSigGenNone(FM) PS##FM##_##SIGGEN_NONE

#define CommonReadyCB(FM) xCommonReadyCB(FM)
#define xCommonReadyCB(FM) ps##FM##BlockReady

const RANGE_INFO_T CommonClass(SCOPE_FAMILY_LT)::rangeInfo[] =
{
    {0.010, 0.5, 0.0, L"10 mV"},
    {0.020, 0.4, 2.0, L"20 mV"},
    {0.050, 0.5, 2.5, L"50 mV"},
    {0.100, 0.5, 2.0, L"100 mV"},
    {0.200, 0.4, 2.0, L"200 mV"},
    {0.500, 0.5, 2.5, L"500 mV"},
    {1.0, 0.5, 2.0, L"1 V"},
    {2.0, 0.4, 2.0, L"2 V"},
    {5.0, 0.5, 2.5, L"5 V"},
    {10.0, 0.5, 2.0, L"10 V"},
    {20.0, 0.0, 2.0, L"20 V"}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common Constructor
//
// Purpose: Handles some of the object initialization
//
// Parameters: [in] _handle - Handle to the scope as defined by the PicoScope driver
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

CommonCtor(SCOPE_FAMILY_LT)( int16_t _handle ) : PicoScope()
{
    handle = _handle;
    initialized = false;
#if !defined(NEW_PS_DRIVER_MODEL)
    hCheckStatusEvent = NULL;
    hCheckStatusThread = NULL;
    readyCB = NULL;
    currentTimeIndisposedMs = 0;
    cbParam = NULL;
#endif
    minRange = 0;
    maxRange = 0;
    timebaseNoiseRejectMode = 0;
    fSampNoiseRejectMode = 0.0;
    signalGeneratorPrecision = 0.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common Destructor
//
// Purpose: Handles object cleanup
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

CommonDtor(SCOPE_FAMILY_LT)()
{
    if (handle != -1)
    { 
        Close();
    }
#if !defined(NEW_PS_DRIVER_MODEL)
    if (hCheckStatusEvent)
    {
        CloseHandle(hCheckStatusEvent);
    }
    if (hCheckStatusThread)
    {
        CloseHandle(hCheckStatusThread);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method Initialized
//
// Purpose: Initialize scope parameters
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT,Initialized)( void )
{
    return (handle != -1 && initialized);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common accessors
//
// Purpose: Get scope parameters
//
// Parameters: Various
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t CommonMethod(SCOPE_FAMILY_LT,GetNumChannels)( void )
{
    return numChannels;
}

uint32_t CommonMethod(SCOPE_FAMILY_LT,GetNoiseRejectModeTimebase)( void )
{
    return timebaseNoiseRejectMode;
}

double CommonMethod(SCOPE_FAMILY_LT,GetNoiseRejectModeSampleRate)( void )
{
    return fSampNoiseRejectMode;
}

double CommonMethod(SCOPE_FAMILY_LT,GetSignalGeneratorPrecision)( void )
{
    return signalGeneratorPrecision;
}

PS_RANGE CommonMethod(SCOPE_FAMILY_LT,GetMinRange)( void )
{
    return minRange;
}

PS_RANGE CommonMethod(SCOPE_FAMILY_LT,GetMaxRange)( void )
{
    return maxRange;
}

int16_t CommonMethod(SCOPE_FAMILY_LT,GetMaxValue)(void)
{
    short maxValue = 0;

#if defined(PS4000)
    if (model == PS4262)
    {
        maxValue = PS4262_MAX_VALUE;
    }
    else
    {
        maxValue = PS4000_MAX_VALUE;
    }
#elif defined(PS2000)
    maxValue = PS2000_MAX_VALUE;
#elif defined(PS3000)
    maxValue = PS3000_MAX_VALUE;
#else
    (void)CommonApi(SCOPE_FAMILY_LT,MaximumValue)( handle, &maxValue );
#endif
    return maxValue;
}

double CommonMethod(SCOPE_FAMILY_LT,GetMinFuncGenFreq)( void )
{
    return minFuncGenFreq;
}

double CommonMethod(SCOPE_FAMILY_LT,GetMaxFuncGenFreq)( void )
{
    return maxFuncGenFreq;
}

double CommonMethod(SCOPE_FAMILY_LT,GetMinFuncGenVpp)( void )
{
    return minFuncGenVpp;
}

double CommonMethod(SCOPE_FAMILY_LT,GetMaxFuncGenVpp)( void )
{
    return maxFuncGenVpp;
}

bool CommonMethod(SCOPE_FAMILY_LT,IsCompatible)( void )
{
    return compatible;
}

const RANGE_INFO_T* CommonMethod(SCOPE_FAMILY_LT,GetRangeCaps)( void )
{
    return rangeInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method GetModel
//
// Purpose: Gets the model number of the scope
//
// Parameters: [out] model - wide string to return the scope model
//             [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined NEW_PS_DRIVER_MODEL
#define INFO_STRING_LENGTH_ARG , &infoStringLength
#else
#define INFO_STRING_LENGTH_ARG
#endif

bool CommonMethod(SCOPE_FAMILY_LT,GetModel)( wstring &model )
{
    bool retVal;
    PICO_STATUS status;
#if defined(NEW_PS_DRIVER_MODEL)
    int16_t infoStringLength;
#endif
    int8_t scopeModel[32];
    if( PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)( handle, scopeModel, sizeof(scopeModel) INFO_STRING_LENGTH_ARG, PICO_VARIANT_INFO )) )
    {
        model = L"???";
        retVal = false;
    }
    else
    {
        wstringstream ssScopeModel;
        ssScopeModel << (char*)scopeModel;
        model.assign(ssScopeModel.str());
        retVal = true;
    }
    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method GetSerialNumber
//
// Purpose: Gets the serial number of the scope
//
// Parameters: [out] sn - wide string to return the scope serial number
//             [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT,GetSerialNumber)( wstring &sn )
{
    bool retVal;
    PICO_STATUS status;
#if defined(NEW_PS_DRIVER_MODEL)
    int16_t infoStringLength;
#endif
    int8_t scopeSN[32];
    if( PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)( handle, scopeSN, sizeof(scopeSN) INFO_STRING_LENGTH_ARG, PICO_BATCH_AND_SERIAL )) )
    {
        sn = L"???";
        retVal = false;
    }
    else
    {
        wstringstream ssScopeSN;
        ssScopeSN << (char*)scopeSN;
        sn.assign(ssScopeSN.str());
        retVal = true;
    }
    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method SetupChannel
//
// Purpose: Sets a scope's sampling channel settings
//
// Parameters: [in] channel - channel number
//             [in] coupling - AC or DC coupling
//             [in] range - voltage range
//             [in] offset - DC offset
//             [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(PS2000A) || defined(PS3000A) || defined(PS5000A)
#define ANALOG_OFFSET_ARG , (float)offset
#elif defined(PS4000) || defined(PS2000) || defined(PS3000)
#define ANALOG_OFFSET_ARG
#endif

bool CommonMethod(SCOPE_FAMILY_LT, SetupChannel)( PS_CHANNEL channel, PS_COUPLING coupling, PS_RANGE range, float offset )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetChannel)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                           TRUE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))coupling,
                                                           (CommonEnum(SCOPE_FAMILY_UT,RANGE))range ANALOG_OFFSET_ARG)))
    {
        fraStatusText << L"Fatal error: Failed to setup input channel: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

#if defined(PS5000A)
    if (retVal && 0 != (status = ps5000aSetBandwidthFilter( handle, (PS5000A_CHANNEL)channel, PS5000A_BW_20MHZ )))
    {
        fraStatusText << L"Fatal error: Failed to setup input channel bandwidth limiter: " << status;
        LogMessage( fraStatusText.str() );
        return false;
    }
#elif defined(PS4000)
    if (model == PS4262)
    {
        if (retVal && 0 != (status = ps4000SetBwFilter( handle, (PS4000_CHANNEL)channel, TRUE )))
        {
            fraStatusText << L"Fatal error: Failed to setup input channel bandwidth limiter: " << status;
            LogMessage( fraStatusText.str() );
            return false;
        }
    }
#endif

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method DisableChannel
//
// Purpose: Disable a channel
//
// Parameters: [in] channel - channel to be disabled 
//             [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, DisableChannel)( PS_CHANNEL channel )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;
    float offset = 0.0;

    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetChannel)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                           FALSE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))0,
                                                           (CommonEnum(SCOPE_FAMILY_UT,RANGE))0 ANALOG_OFFSET_ARG)))
    {
        fraStatusText << L"Fatal error: Failed to disable channel: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method SetSignalGenerator
//
// Purpose: Setup the signal generator to generate a sine with the given amplitude and frequency
//
// Parameters: [in] vPP - voltage peak-to-peak in microvolts
//             [in] frequency - frequency to generate in Hz
//             [out] return - whether the function succeeded
//
// Notes: vPP is ignored for PS3000 series scopes
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, SetSignalGenerator)( double vPP, double frequency )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;
    
#if defined(PS2000)
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetSigGenBuiltIn)( handle, 0, (uint32_t)(vPP*1.0e6), CommonSine(SCOPE_FAMILY_UT), (float)frequency, (float)frequency,
                                                                 1.0, 1.0, (CommonEnum(SCOPE_FAMILY_UT,SWEEP_TYPE))0, 0 )))
#elif defined(PS3000)
    UNREFERENCED_PARAMETER(vPP);
    // Truncation of frequency to 0 will disable the generator.  However, for PS3000 this function should never be called with values less than 100 Hz, unless
    // actually attempting to disable.
    int32_t intFreq = saturation_cast<int32_t,double>(frequency);
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, _set_siggen)( handle, CommonSine(SCOPE_FAMILY_UT), intFreq, intFreq, 1.0, 1, 0, 0 )))
#else
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetSigGenBuiltIn)( handle, 0, (uint32_t)(vPP*1.0e6), CommonSine(SCOPE_FAMILY_UT), (float)frequency, (float)frequency,
                                                                 1.0, 1.0, (CommonEnum(SCOPE_FAMILY_UT,SWEEP_TYPE))0,
                                                                 CommonEsOff(SCOPE_FAMILY_UT), 
                                                                 0, 0, (CommonEnum(SCOPE_FAMILY_UT,SIGGEN_TRIG_TYPE))0,
                                                                 (CommonEnum(SCOPE_FAMILY_UT,SIGGEN_TRIG_SOURCE))CommonSigGenNone(SCOPE_FAMILY_UT), 0 )))
#endif
    {
        fraStatusText << L"Fatal error: Failed to setup stimulus signal: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
#if defined(PS3000)
    if (status != intFreq)
    {
        fraStatusText << L"Fatal error: Intended and actual stimulus signal frequency don't match: Intended = " << intFreq << ", Actual = " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
#endif

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method DisableSignalGenerator
//
// Purpose: Disable the signal generator
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, DisableSignalGenerator)( void )
{
    bool retVal = true;
    wstringstream fraStatusText;
#if defined(PS3000)
    if (!SetSignalGenerator( 0.0, 0.0 )) // Set to 0.0 Hz, volts peak to peak are ignored
#else
    if (!SetSignalGenerator( 0.0, GetMinFuncGenFreq() )) // Set to 0.0 volts and minimum frequency;
#endif
    {
        fraStatusText << L"Error: Failed to disable signal generator.";
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method GetMaxSamples
//
// Purpose: Gets the maximum possible number of samples per channel per the scope's reported
//          buffer capabilities
//
// Parameters: [out] pMaxSamples - the maximum samples per channel
//             [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(PS5000A)
#define TIME_UNITS_ARG
#define OVERSAMPLE_ARG
#define SEGMENT_INDEX_ARG , 0
#elif defined(PS2000A) || defined(PS3000A) || defined(PS4000)
#define TIME_UNITS_ARG
#define OVERSAMPLE_ARG 1,
#define SEGMENT_INDEX_ARG , 0
#elif defined(PS2000) || defined(PS3000)
#define TIME_UNITS_ARG NULL,
#define OVERSAMPLE_ARG 1,
#define SEGMENT_INDEX_ARG
#endif

bool CommonMethod(SCOPE_FAMILY_LT, GetMaxSamples)( uint32_t* pMaxSamples )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

#if defined(PS6000)
    uint32_t maxSamples;
#else
    int32_t maxSamples;
#endif

    // timebase should not affect number of samples, so just set to lowest possible value common to all scopes/modes
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, GetTimebase)( handle, 3, 0, NULL, TIME_UNITS_ARG OVERSAMPLE_ARG &maxSamples SEGMENT_INDEX_ARG )))
    {
        fraStatusText << L"Fatal error: Failed to determine sample buffer size: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
    else
    {
        *pMaxSamples = maxSamples;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method DisableChannelTriggers
//
// Purpose: Disables any triggers for the scope's sampling channels
//
// Parameters: [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, DisableChannelTriggers)( void )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

#if defined(NEW_PS_DRIVER_MODEL)
    // It's possible that only one of these is necessary.  But it's probably good practice to take some action to disable
    // triggers rather than rely on a scopes default state.
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetTriggerChannelConditions)( handle, NULL, 0 )))
    {
        fraStatusText << L"Fatal error: Failed to setup channel trigger conditions: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetTriggerChannelProperties)( handle, NULL, 0, 0, 0 )))
    {
        fraStatusText << L"Fatal error: Failed to setup channel trigger properties: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
#else
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, _set_trigger)( handle, CommonEnum(SCOPE_FAMILY_UT, NONE), 0, CommonEnum(SCOPE_FAMILY_UT, RISING), 0, 0 )))
    {
        fraStatusText << L"Fatal error: Failed to setup channel trigger properties: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
#endif

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method RunBlock
//
// Purpose: Starts a sample capture on the scope using block mode
//
// Parameters: [in] numSamples - number of samples to capture
//             [in] timebase - sampling timebase for the scope
//             [out] timeIndisposedMs - estimated time it will take to capture the block of data 
//                                      in milliseconds
//             [in] lpReady - pointer to the callback function to be called when the block
//                            has been captured
//             [in] pParameter - parameter to be based to the callback function
//             [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, RunBlock)( int32_t numSamples, uint32_t timebase, int32_t *timeIndisposedMs, psBlockReady lpReady, void *pParameter )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

    // Setup block mode
#if defined(NEW_PS_DRIVER_MODEL)
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, RunBlock)( handle, 0, numSamples, timebase, OVERSAMPLE_ARG timeIndisposedMs, 0,
                                                         (CommonReadyCB(SCOPE_FAMILY_LT))lpReady, pParameter)))
#else
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, _run_block)( handle, numSamples, timebase, 1, timeIndisposedMs )))
#endif
    {
        fraStatusText << L"Fatal error: Failed to start channel data capture: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
#if !defined(NEW_PS_DRIVER_MODEL)
    else
    {
        currentTimeIndisposedMs = *timeIndisposedMs;
        readyCB = lpReady;
        cbParam = pParameter;
        if (!SetEvent( hCheckStatusEvent ))
        {
            fraStatusText << L"Fatal error: Failed to start channel data capture: Failed to start ready status checking.";
            LogMessage( fraStatusText.str() );
            retVal = false;
        }
    }
#endif

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method CheckStatus
//
// Purpose: Used for scopes with driver families preceding the new driver model (e.g. PS2000 and
//          PS3000) which don't support block mode completion callbacks.  This is a thread function 
//          that supports an emulation of the callback feature present in the new driver model.
//
// Parameters: [in] lpThreadParameter - parameter passed to the thread function.  Here it's a
//                                      pointer to the object instance.
//             [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(NEW_PS_DRIVER_MODEL)
DWORD WINAPI CommonMethod(SCOPE_FAMILY_LT, CheckStatus)(LPVOID lpThreadParameter)
{
    int16_t readyStatus;
    PICO_STATUS status;
    int32_t delayCounter;
    CommonClass(SCOPE_FAMILY_LT)* inst = (CommonClass(SCOPE_FAMILY_LT)*)lpThreadParameter;
    do
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject( inst->hCheckStatusEvent, INFINITE ))
        {
            // Check that the handle is valid and implement a delay limit here, because unfortunately the ready function return
            // does not distinguish between an invalid handle and not ready
            if (inst->handle > 0)
            {
                delayCounter = 0;
                while (0 == (readyStatus = CommonApi(SCOPE_FAMILY_LT, _ready)(inst->handle)))
                {
                    Sleep(100);
                    delayCounter += 100;
                    // delay with a safety factor of 1.5x and never let it go less than 3 seconds
                    if (delayCounter >= max( 3000, ((inst->currentTimeIndisposedMs)*3)/2 ))
                    {
                        break;
                    }
                }
                if (readyStatus == 0)
                {
                    continue; // Don't call the callback to help avoid races between this expiry and the one in PicoScopeFRA
                }
                if (readyStatus < 0)
                {
                    status = PICO_DATA_NOT_AVAILABLE;
                }
                else
                {
                    status = PICO_OK;
                }
            }
            else
            {
                status = PICO_DATA_NOT_AVAILABLE;
            }
            // Call the callback
            inst->readyCB(inst->handle, status, inst->cbParam);
        }
        else
        {
            // Soemthing has gone catastrophically wrong, so just exit the thread.  At this point no callbacks will work
            // and the upper level code should detect the failure by timing out.
            wstringstream fraStatusText;
            fraStatusText << L"Fatal error: Invalid result from waiting on status checking start event";
            LogMessage( fraStatusText.str() );
            return -1;
        }

    } while (1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method InitStatusChecking
//
// Purpose: Initializes the event and thread supporting status checking and block mode callbacks
//
// Parameters: [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, InitStatusChecking)(void)
{
    bool retVal = true;
    if (NULL == (hCheckStatusEvent = CreateEvent( NULL, false, false, NULL )))
    {
    retVal = false;
    }
    else if (NULL == (hCheckStatusThread = CreateThread( NULL, 0, CheckStatus, this, 0, NULL )))
    {
        retVal = false;
    }
    return retVal;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method GetData
//
// Purpose: Gets data from the scope that was captured in block mode.
//
// Parameters: [in] numSamples - number of samples to retrieve in the uncompressed buffer
//             [in] downsampleTo - number of samples to store in a compressed aggregate buffer
//             [in] inputChannel - input channel number
//             [in] outputChannel - output channel number
//             [in] inputFullBuffer - buffer to store uncompressed input samples
//             [in] outputFullBuffer - buffer to store uncompressed output samples
//             [in] inputCompressedMinBuffer - buffer to store aggregated min input samples
//             [in] outputCompressedMinBuffer - buffer to store aggregated min ouput samples
//             [in] inputCompressedMaxBuffer - buffer to store aggregated max input samples
//             [in] outputCompressedMaxBuffer - buffer to store aggregated max ouput samples
//             [in] inputAbsMax - absolute max value in all input samples
//             [in] outputAbsMax - absolute max value in all output samples
//             [in] inputOv - whether the input channel is over-voltage
//             [in] outputOv - whether the output channel is over-voltage 
//             [out] return - whether the function succeeded
//
// Notes: PS2000 and PS3000 drivers support an aggregation mode that works with "fast streaming"
//        which may be able to mimic block mode (using auto_stop).  None the the PS3000 scopes that
//        support fast streaming also have a signal generator, so are irrelevant.  All of the 
//        compatible PS2000 scopes do support fast streaming.  However, given the relatively 
//        smaller size of the buffers these scopes use, I decided to favor code simplicity over
//        speed and just use block mode which has no aggregation.  The aggregation will be done
//        in software on the PC-side.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(PS2000A) || defined(PS3000A) || defined(PS5000A)
#define RATIO_MODE_NONE_ARG , CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_NONE)
#define RATIO_MODE_AGGREGATE_ARG , CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_AGGREGATE)
#define SEGMENT_ARG , 0
#elif defined(PS4000)
#define RATIO_MODE_NONE_ARG
#define RATIO_MODE_AGGREGATE_ARG
#define SEGMENT_ARG
#endif

// GetData
bool CommonMethod(SCOPE_FAMILY_LT, GetData)( uint32_t numSamples, uint32_t downsampleTo, PS_CHANNEL inputChannel, PS_CHANNEL outputChannel,
                                             vector<int16_t>& inputFullBuffer, vector<int16_t>& outputFullBuffer,
                                             vector<int16_t>& inputCompressedMinBuffer, vector<int16_t>& outputCompressedMinBuffer,
                                             vector<int16_t>& inputCompressedMaxBuffer, vector<int16_t>& outputCompressedMaxBuffer, 
                                             int16_t& inputAbsMax, int16_t& outputAbsMax, bool* inputOv, bool* outputOv )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

    uint32_t compressedBufferSize;
    uint32_t initialAggregation;
    int16_t overflow;

    // For NEW_PS_DRIVER_MODEL: Determine whether the channels are overvoltage and get the downsampled (compressed) buffer in one step by using the scope's aggregation function
    // For non-NEW_PS_DRIVER_MODEL: We're calculating an aggregation parameter for our own manual aggregation 

    // Determine the aggregation parameter, rounding up to ensure that the actual size of the compressed is no more than requested
    compressedBufferSize = (downsampleTo == 0 || numSamples <= downsampleTo) ? numSamples : downsampleTo;
    initialAggregation = numSamples / compressedBufferSize;
    if (numSamples % compressedBufferSize)
    {
        initialAggregation++;
    }
    // Now compute actual size of aggregate buffer
    compressedBufferSize = numSamples / initialAggregation;

    // And allocate the data for the compressed buffers
    inputCompressedMinBuffer.resize(compressedBufferSize);
    outputCompressedMinBuffer.resize(compressedBufferSize);
    inputCompressedMaxBuffer.resize(compressedBufferSize);
    outputCompressedMaxBuffer.resize(compressedBufferSize);

#if defined(NEW_PS_DRIVER_MODEL)
    uint32_t numSamplesInOut;

    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))inputChannel,
                                                               inputCompressedMaxBuffer.data(), inputCompressedMinBuffer.data(), 
                                                               compressedBufferSize SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG )))
    {
        fraStatusText << L"Fatal error: Failed to set input data capture buffer: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))outputChannel,
                                                               outputCompressedMaxBuffer.data(), outputCompressedMinBuffer.data(), 
                                                               compressedBufferSize SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG )))
    {
        fraStatusText << L"Fatal error: Failed to set output data capture buffer: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    numSamplesInOut = compressedBufferSize;
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, GetValues)( handle, 0, &numSamplesInOut, initialAggregation, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_AGGREGATE), 0, &overflow )))
    {
        fraStatusText << L"Fatal error: Failed to retrieve data capture buffer for OV detection: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
#else // !defined(NEW_PS_DRIVER_MODEL)

    // Just use block mode without aggregation to get all the data.

    int16_t* buffer[PS_CHANNEL_D+1] = {NULL};

    buffer[inputChannel] = inputFullBuffer.data();
    buffer[outputChannel] = outputFullBuffer.data();

    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, _get_values)( handle, buffer[PS_CHANNEL_A], buffer[PS_CHANNEL_B], buffer[PS_CHANNEL_C], buffer[PS_CHANNEL_D], &overflow, numSamples )))
    {
        fraStatusText << L"Fatal error: Failed to retrieve data capture buffer: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    // Now manually aggregate, if necessary
    if (compressedBufferSize == numSamples)
    {
        inputCompressedMinBuffer = inputFullBuffer;
        inputCompressedMaxBuffer = inputFullBuffer;
        outputCompressedMinBuffer = outputFullBuffer;
        outputCompressedMaxBuffer = outputFullBuffer;
    }
    else
    {
        uint32_t i;
        for (i = 0; i < compressedBufferSize-1; i++)
        {
            auto minMaxIn = minmax_element( &inputFullBuffer[i*initialAggregation], &inputFullBuffer[(i+1)*initialAggregation] );
            inputCompressedMinBuffer[i] = *minMaxIn.first;
            inputCompressedMaxBuffer[i] = *minMaxIn.second;
            auto minMaxOut = minmax_element( &outputFullBuffer[i*initialAggregation], &outputFullBuffer[(i+1)*initialAggregation] );
            outputCompressedMinBuffer[i] = *minMaxOut.first;
            outputCompressedMaxBuffer[i] = *minMaxOut.second;
        }
        // Handle the last few samples
        auto minMaxIn = minmax_element( &inputFullBuffer[i*initialAggregation], &inputFullBuffer[numSamples] );
        inputCompressedMinBuffer[i] = *minMaxIn.first;
        inputCompressedMaxBuffer[i] = *minMaxIn.second;
        auto minMaxOut = minmax_element( &outputFullBuffer[i*initialAggregation], &outputFullBuffer[numSamples] );
        outputCompressedMinBuffer[i] = *minMaxOut.first;
        outputCompressedMaxBuffer[i] = *minMaxOut.second;
    }
#endif

    // Decode overflow
    *inputOv = ((overflow & 1<<inputChannel) != 0);
    *outputOv = ((overflow & 1<<outputChannel) != 0);

#if defined(NEW_PS_DRIVER_MODEL)
    if (compressedBufferSize == numSamples)
    {
        // Simply copy the aggregated samples to the full buffer. It doesn't matter which since they should be the same.
        inputFullBuffer = inputCompressedMaxBuffer;
        outputFullBuffer = outputCompressedMaxBuffer;
    }
    else
    {
        // Setup buffers only for channels not overvoltage, to optimize transfer

        // This code works, but still takes the same amount of time as transferring all samples.
        // This is true in the first few attempts before we ever tell it about one of the buffers
        // (input in the case of the HPF).  So it's like the lower level driver and scope aren't
        // capable of transferring just one channel at a time.  Confirmed this with support, case
        // number TS00062825.  Logged a feature request to allow only single channel transfer.
        // It remains a valid optimization in the case where both channels are overvoltage.
        if (!(*inputOv))
        {
            if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))inputChannel, inputFullBuffer.data(),
                                                                     numSamples SEGMENT_ARG RATIO_MODE_NONE_ARG )))
            {
                fraStatusText << L"Fatal error: Failed to set input data capture buffer: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }
        else
        {
            if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))inputChannel, NULL,
                                                                     0 SEGMENT_ARG RATIO_MODE_NONE_ARG )))
            {
                fraStatusText << L"Fatal error: Failed to set input data capture buffer: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }

        if (!(*outputOv))
        {
            if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))outputChannel, outputFullBuffer.data(),
                                                                     numSamples SEGMENT_ARG RATIO_MODE_NONE_ARG )))
            {
                fraStatusText << L"Fatal error: Failed to set output data capture buffer: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }
        else
        {
            if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))outputChannel, NULL,
                                                                     0 SEGMENT_ARG RATIO_MODE_NONE_ARG )))
            {
                fraStatusText << L"Fatal error: Failed to set output data capture buffer: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }

        // Only transfer data if at least one of the channels is not over-voltage
        if (!(*inputOv) || !(*outputOv))
        {
            numSamplesInOut = numSamples;
            if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, GetValues)( handle, 0, &numSamplesInOut, 1, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_NONE), 0, &overflow )))
            {
                fraStatusText << L"Fatal error: Failed to retrieve data capture buffer: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }
    }
#endif

    // Get overall min and max values
    // Could use the scope to aggregate down to 1 min/max sample, but the compressed buffer is so small this way is as fast or faster.
    inputAbsMax = abs(*min_element(inputCompressedMinBuffer.begin(), inputCompressedMinBuffer.end()));
    inputAbsMax = max(inputAbsMax, abs(*max_element(inputCompressedMaxBuffer.begin(), inputCompressedMaxBuffer.end())));

    outputAbsMax = abs(*min_element(outputCompressedMinBuffer.begin(), outputCompressedMinBuffer.end()));
    outputAbsMax = max(outputAbsMax, abs(*max_element(outputCompressedMaxBuffer.begin(), outputCompressedMaxBuffer.end())));

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method Close
//
// Purpose: Close the scope
//
// Parameters: [out] return - whether the function succeeded
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, Close)(void)
{
    PICO_STATUS status;
    status = CommonApi(SCOPE_FAMILY_LT, CloseUnit)( handle );
    handle = -1;
    return (PICO_OK == status);
}

#undef GetUnitInfo
#undef SetChannel
#undef SetSigGenBuiltIn
#undef GetTimebase
#undef SetTriggerChannelConditions
#undef CloseUnit