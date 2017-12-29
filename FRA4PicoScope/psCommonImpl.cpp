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
#include <boost/math/special_functions/round.hpp>
using namespace boost::math;

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
#elif defined(PS4000A)
#define SCOPE_FAMILY_LT 4000a
#define SCOPE_FAMILY_UT 4000A
#define NEW_PS_DRIVER_MODEL
#elif defined(PS5000)
#define SCOPE_FAMILY_LT 5000
#define SCOPE_FAMILY_UT 5000
#define NEW_PS_DRIVER_MODEL
#elif defined(PS5000A)
#define SCOPE_FAMILY_LT 5000a
#define SCOPE_FAMILY_UT 5000A
#define NEW_PS_DRIVER_MODEL
#elif defined(PS6000)
#define SCOPE_FAMILY_LT 6000
#define SCOPE_FAMILY_UT 6000
#define NEW_PS_DRIVER_MODEL
#endif

#define LOG_PICO_API_CALL(...) \
fraStatusText.clear(); \
fraStatusText.str(L""); \
LogPicoApiCall(fraStatusText, __VA_ARGS__);

// NEW_PS_DRIVER_MODEL means that the driver API:
// 1) Supports capture completion callbacks
// 2) Returns status with PICO_OK indicating success
// 3) Normally uses CamelCase identifiers
#if defined(NEW_PS_DRIVER_MODEL)
#define PICO_ERROR(x) (status = x) == PICO_OK ? 0 : (PICO_POWER_SUPPLY_CONNECTED == status || PICO_POWER_SUPPLY_NOT_CONNECTED == status) ? throw PicoPowerChange(status) : 1
#define PICO_CONNECTION_ERROR(x) (status = x) == PICO_OK ? 0 : (PICO_BUSY == status) ? 0 : 1
#else
#define PICO_ERROR(x) 0 == (status = x)
#define PICO_CONNECTION_ERROR(x) 0 == (status = x)
#define GetUnitInfo _get_unit_info
#define SetChannel _set_channel
#define SetSigGenBuiltIn _set_sig_gen_built_in
#define GetTimebase _get_timebase
#define SetTriggerChannelConditions _set_trigger
#define Stop _stop
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

#define CommonDC(FM) xCommonDC(FM)
#define xCommonDC(FM) PS##FM##_##DC_VOLTAGE

#define CommonEsOff(FM) xCommonEsOff(FM)
#define xCommonEsOff(FM) PS##FM##_##ES_OFF

#define CommonSigGenNone(FM) xCommonSigGenNone(FM)
#define xCommonSigGenNone(FM) PS##FM##_##SIGGEN_NONE

#define CommonReadyCB(FM) xCommonReadyCB(FM)
#define xCommonReadyCB(FM) ps##FM##BlockReady

#define CommonErrorCode(FM) xCommonErrorCode(FM)
#define xCommonErrorCode(FM) PS##FM##_ERROR_CODE

const RANGE_INFO_T CommonClass(SCOPE_FAMILY_LT)::rangeInfo[] =
{
    {0.010, 0.5, 0.0, L"± 10 mV"},
    {0.020, 0.4, 2.0, L"± 20 mV"},
    {0.050, 0.5, 2.5, L"± 50 mV"},
    {0.100, 0.5, 2.0, L"± 100 mV"},
    {0.200, 0.4, 2.0, L"± 200 mV"},
    {0.500, 0.5, 2.5, L"± 500 mV"},
    {1.0, 0.5, 2.0, L"± 1 V"},
    {2.0, 0.4, 2.0, L"± 2 V"},
    {5.0, 0.5, 2.5, L"± 5 V"},
    {10.0, 0.5, 2.0, L"± 10 V"},
    {20.0, 0.0, 2.0, L"± 20 V"}
};

// It is imperative that this value be greater than (half) the largest buffer in the scope family for
// scopes not implementing the new driver model.  Currently this is 1MB (PS3206)
const uint32_t CommonClass(SCOPE_FAMILY_LT)::maxDataRequestSize = 16 * 1024 * 1024; // 16 MSamp (32 MB)

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common Constructor
//
// Purpose: Handles some of the object initialization
//
// Parameters: [in] _handle - Handle to the scope as defined by the PicoScope driver
//             [in] _initialPowerState - the power state returned by open function
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(PS3000A) || defined(PS4000A) || defined(PS5000A)
CommonCtor(SCOPE_FAMILY_LT)( int16_t _handle, PICO_STATUS _initialPowerState ) : PicoScope()
#else
CommonCtor(SCOPE_FAMILY_LT)( int16_t _handle ) : PicoScope()
#endif
{
    handle = _handle;
    initialized = false;
#if !defined(NEW_PS_DRIVER_MODEL)
    hCheckStatusEvent = NULL;
    hCheckStatusThread = NULL;
    capturing = false;
    readyCB = NULL;
    currentTimeIndisposedMs = 0;
    cbParam = NULL;
#endif
    minRange = 0;
    maxRange = 0;
    minTimebase = 0;
    maxTimebase = 0;
    timebaseNoiseRejectMode = 0;
    defaultTimebaseNoiseRejectMode = 0;
    signalGeneratorPrecision = 0.0;
    mInputChannel = PS_CHANNEL_INVALID;
    mOutputChannel = PS_CHANNEL_INVALID;

    uint32_t bufferSize;
    GetMaxSamples( &bufferSize );
    bufferSize = min( maxDataRequestSize, bufferSize );
    mInputBuffer.resize( bufferSize );
    mOutputBuffer.resize( bufferSize );
    mNumSamples = 0;
    buffersDirty = true;
#if defined(PS3000A) || defined(PS4000A) || defined(PS5000A)
    numActualChannels = 0;
    initialPowerState = _initialPowerState;
#endif;
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
// Purpose: Indicate if the scope has been initialized
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
// Name: Common method Connected
//
// Purpose: Detect if the scope is connected
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT,Connected)( void )
{
    PICO_STATUS status;
    wstringstream fraStatusText;

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, PingUnit)), handle );
    status = CommonApi(SCOPE_FAMILY_LT, PingUnit)( handle );
#if defined(NEW_PS_DRIVER_MODEL)
    return !(PICO_CONNECTION_ERROR(status));
#else
    // TODO: finalize this code.  Right now it's just here to diagnose issues with user initiated cancel on PS2000
    if (PICO_CONNECTION_ERROR(status))
    {
        int8_t lastError[16];
        LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)), handle, lastError, sizeof(lastError), CommonErrorCode(SCOPE_FAMILY_UT) );
        CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)( handle, lastError, sizeof(lastError), CommonErrorCode(SCOPE_FAMILY_UT) );
        return false;
    }
    else
    {
        return true;
    }
#endif
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
#if defined(PS3000A) || defined(PS4000A) || defined(PS5000A)
    if (numActualChannels > 2)
    {
        PICO_STATUS currentPowerState;
        wstringstream fraStatusText;
        LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, CurrentPowerSource)), handle );
        currentPowerState = CommonApi(SCOPE_FAMILY_LT, CurrentPowerSource)(handle);

        if (IsUSB3_0Connection() || PICO_POWER_SUPPLY_CONNECTED == currentPowerState )
        {
            numAvailableChannels = numActualChannels;
        }
        else if (PICO_POWER_SUPPLY_NOT_CONNECTED == currentPowerState)
        {
            numAvailableChannels = 2;
        }
        else // The handle is invalid, so just set a default presuming an error will get caught later.
        {
            numAvailableChannels = numActualChannels;
        }
    }
#endif
    return numAvailableChannels;
}

void CommonMethod(SCOPE_FAMILY_LT,GetAvailableCouplings)( vector<wstring>& couplingText )
{
#if defined (PS6000)
    if (model == PS6407)
    {
        couplingText.clear();
        couplingText.resize(1);
        couplingText[0] = TEXT("DC 50R");
    }
    else
    {
        couplingText.clear();
        couplingText.resize(3);
        couplingText[0] = TEXT("AC");
        couplingText[1] = TEXT("DC 1M");
        couplingText[2] = TEXT("DC 50R");
    }
#else
    couplingText.clear();
    couplingText.resize(2);
    couplingText[0] = TEXT("AC");
    couplingText[1] = TEXT("DC");
#endif
}

uint32_t CommonMethod(SCOPE_FAMILY_LT,GetMinTimebase)( void )
{
    return minTimebase;
}

uint32_t CommonMethod(SCOPE_FAMILY_LT,GetMaxTimebase)( void )
{
    return maxTimebase;
}

void CommonMethod(SCOPE_FAMILY_LT,SetDesiredNoiseRejectModeTimebase)( uint32_t timebase )
{
    timebaseNoiseRejectMode = timebase;
}

uint32_t CommonMethod(SCOPE_FAMILY_LT,GetDefaultNoiseRejectModeTimebase)( void )
{
    return defaultTimebaseNoiseRejectMode;
}

uint32_t CommonMethod(SCOPE_FAMILY_LT,GetNoiseRejectModeTimebase)( void )
{
    if (model == PS6407)
    {
        if (((mInputChannel == PS_CHANNEL_A || mInputChannel == PS_CHANNEL_B) &&
             (mOutputChannel == PS_CHANNEL_A || mOutputChannel == PS_CHANNEL_B)) ||
            ((mInputChannel == PS_CHANNEL_C || mInputChannel == PS_CHANNEL_D) &&
             (mOutputChannel == PS_CHANNEL_C || mOutputChannel == PS_CHANNEL_D)))
        {
            return max( 2, timebaseNoiseRejectMode );
        }
    }

    return timebaseNoiseRejectMode;
}

double CommonMethod(SCOPE_FAMILY_LT,GetNoiseRejectModeSampleRate)( void )
{
    double sampleRate;
    GetFrequencyFromTimebase( GetNoiseRejectModeTimebase(), sampleRate );
    return sampleRate;
}

double CommonMethod(SCOPE_FAMILY_LT,GetSignalGeneratorPrecision)( void )
{
    return signalGeneratorPrecision;
}

uint32_t CommonMethod(SCOPE_FAMILY_LT,GetMaxDataRequestSize)(void)
{
    return maxDataRequestSize;
}

PS_RANGE CommonMethod(SCOPE_FAMILY_LT,GetMinRange)( PS_COUPLING coupling )
{
    UNREFERENCED_PARAMETER(coupling);
    return minRange;
}

PS_RANGE CommonMethod(SCOPE_FAMILY_LT,GetMaxRange)( PS_COUPLING coupling )
{
#if defined (PS6000)
    if (coupling == PS_DC_50R)
    {
        return (maxRange - 2);
    }
    else
    {
        return maxRange;
    }
#else
    UNREFERENCED_PARAMETER(coupling);
    return maxRange;
#endif
}

int16_t CommonMethod(SCOPE_FAMILY_LT,GetMaxValue)(void)
{
    short maxValue = 0;
    wstringstream fraStatusText;

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
#elif defined(PS5000)
    maxValue = PS5000_MAX_VALUE;
#elif defined(PS6000)
    maxValue = PS6000_MAX_VALUE;
#else
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT,MaximumValue)), handle, &maxValue );
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

double CommonMethod(SCOPE_FAMILY_LT,GetMinNonZeroFuncGenVpp)( void )
{
    // Current API for setting the signal generator has a resolution of 1 uV.
    // So, to avoid rounding the value to 0, add epsilon
    return max( minFuncGenVpp, 1e-6+std::numeric_limits<double>::epsilon() );
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
// Name: Common method GetClosestSignalGeneratorFrequency
//
// Purpose: Calculates the frequency closest to the requested frequency that the signal generator
//          is capable of generating.
//
// Parameters: [in] requestedFreq - desired frequency
//             [out] return - closest frequency the scope is capable of generating.
//
// Notes: Frequency generator capabilities are governed by the scope's DDS implementation.  The
//        frequency precision is encoded in the PicoScope object's implementation (signalGeneratorPrecision).
//
//        For PS3000, we need to implement special logic because the API takes an integer, but
//        the scope rounds that to the closest DDS-producable frequency (which is not an integer).
//        The frequency precision is normally far smaller than 1 (true for PS3000).  So, we find
//        the two integers adjacent to the requested frequency, then find the actual frequencies to
//        which those integers will get rounded by the API/scope.  Then we find which of those
//        actual frequencies is closest to the requested frequency.  When the program uses the
//        returned value to setup the generator, which is a double, the value will get truncated
//        back to an integer before passing to the API, but the API/scope will implement the
//        actual frequency desired.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

double CommonMethod(SCOPE_FAMILY_LT,GetClosestSignalGeneratorFrequency)( double requestedFreq )
{
#if defined(PS3000) // Unique because the ps3000_set_siggen function takes integers for frequency
    double actualFreq;
    uint32_t N, M; // integers below and above the requested frequency
    double fClosestToN, fClosestToM; // frequencies closest to N and M that generator is capable of producing
    N = (uint32_t)requestedFreq;
    M = N+1;
    // Find closest signalGeneratorPrecision to each of N and M
    fClosestToN = round((double)N / signalGeneratorPrecision)*signalGeneratorPrecision;
    fClosestToM = round((double)M / signalGeneratorPrecision)*signalGeneratorPrecision;
    actualFreq = fabs(requestedFreq-fClosestToN) < fabs(requestedFreq-fClosestToM) ? fClosestToN : fClosestToM;
#else
    double actualFreq = lround(requestedFreq / signalGeneratorPrecision) * signalGeneratorPrecision;
#endif
    // Bound in case any rounding caused us to go outside the capabilities of the generator
    actualFreq = min(actualFreq, maxFuncGenFreq);
    actualFreq = max(actualFreq, minFuncGenFreq);
    return actualFreq;
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
    wstringstream fraStatusText;
#if defined(NEW_PS_DRIVER_MODEL)
    int16_t infoStringLength;
#endif
    int8_t scopeModel[32];
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)), handle, scopeModel, sizeof(scopeModel) INFO_STRING_LENGTH_ARG, PICO_VARIANT_INFO );
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
    wstringstream fraStatusText;
#if defined(NEW_PS_DRIVER_MODEL)
    int16_t infoStringLength;
#endif
    int8_t scopeSN[32];
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)), handle, scopeSN, sizeof(scopeSN) INFO_STRING_LENGTH_ARG, PICO_BATCH_AND_SERIAL );
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

#if defined(PS2000A) || defined(PS3000A) || defined (PS4000A) || defined(PS5000A) || defined(PS6000)
#define ANALOG_OFFSET_ARG , (float)offset
#elif defined(PS4000) || defined(PS2000) || defined(PS3000) || defined(PS5000)
#define ANALOG_OFFSET_ARG
#endif

#if defined(PS6000)
#define BANDWIDTH_LIMITER_ARG , bwLimiter
#else
#define BANDWIDTH_LIMITER_ARG
#endif

bool CommonMethod(SCOPE_FAMILY_LT, SetupChannel)( PS_CHANNEL channel, PS_COUPLING coupling, PS_RANGE range, float offset )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

#if defined(PS6000)
    PS6000_BANDWIDTH_LIMITER bwLimiter;
    if (model == PS6407)
    {
        bwLimiter = PS6000_BW_FULL; // PS6407 has no bandwidth limiter
    }
    else if (model == PS6404 || model == PS6404A || model == PS6404B || model == PS6404C || model == PS6404D)
    {
        bwLimiter = PS6000_BW_25MHZ;
    }
    else
    {
        bwLimiter = PS6000_BW_20MHZ;
    }
#endif

#if defined(PS4000A)
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetChannel)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                                                   TRUE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))coupling,
                                                                                   (PICO_CONNECT_PROBE_RANGE)range ANALOG_OFFSET_ARG
                                                                                   BANDWIDTH_LIMITER_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetChannel)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                           TRUE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))coupling,
                                                           (PICO_CONNECT_PROBE_RANGE)range ANALOG_OFFSET_ARG
                                                           BANDWIDTH_LIMITER_ARG )))
#else
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetChannel)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                                                   TRUE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))coupling,
                                                                                   (CommonEnum(SCOPE_FAMILY_UT,RANGE))range ANALOG_OFFSET_ARG
                                                                                   BANDWIDTH_LIMITER_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetChannel)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                           TRUE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))coupling,
                                                           (CommonEnum(SCOPE_FAMILY_UT,RANGE))range ANALOG_OFFSET_ARG
                                                           BANDWIDTH_LIMITER_ARG )))
#endif
    {
        fraStatusText << L"Fatal error: Failed to setup input channel: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

#if defined(PS5000A)
    if (retVal)
    {
        LOG_PICO_API_CALL( L"ps5000aSetBandwidthFilter", handle, (PS5000A_CHANNEL)channel, PS5000A_BW_20MHZ );
        if (0 != (status = ps5000aSetBandwidthFilter( handle, (PS5000A_CHANNEL)channel, PS5000A_BW_20MHZ )))
        {
            fraStatusText.clear();
            fraStatusText.str(L"");
            fraStatusText << L"Fatal error: Failed to setup input channel bandwidth limiter: " << status;
            LogMessage( fraStatusText.str() );
            retVal = false;
        }
    }
#elif defined(PS4000)
    if (model == PS4262)
    {
        if (retVal)
        {
            LOG_PICO_API_CALL( L"ps4000SetBwFilter", handle, (PS4000_CHANNEL)channel, TRUE );
            if (0 != (status = ps4000SetBwFilter( handle, (PS4000_CHANNEL)channel, TRUE )))
            {
                fraStatusText.clear();
                fraStatusText.str(L"");
                fraStatusText << L"Fatal error: Failed to setup input channel bandwidth limiter: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }
    }
#elif defined(PS3000A)
    if (model == PS3404A || model == PS3404B || model == PS3405A || model == PS3405B || model == PS3406A || model == PS3406B ||
        model == PS3203D || model == PS3203DMSO || model == PS3204D || model == PS3204DMSO || model == PS3205D || model == PS3205DMSO || model == PS3206D || model == PS3206DMSO ||
        model == PS3403D || model == PS3403DMSO || model == PS3404D || model == PS3404DMSO || model == PS3405D || model == PS3405DMSO || model == PS3406D || model == PS3406DMSO)
    {
        if (retVal)
        {
            LOG_PICO_API_CALL( L"ps3000aSetBandwidthFilter", handle, (PS3000A_CHANNEL)channel, PS3000A_BW_20MHZ );
            if(0 != (status = ps3000aSetBandwidthFilter( handle, (PS3000A_CHANNEL)channel, PS3000A_BW_20MHZ )))
            {
                fraStatusText.clear();
                fraStatusText.str(L"");
                fraStatusText << L"Fatal error: Failed to setup input channel bandwidth limiter: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }
    }
#endif

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method DisableAllDigitalChannels
//
// Purpose: Disable all digital channels
//
// Parameters: [out] return - whether the function succeeded
//
// Notes: When digital channels are on, they can reduce available sampling frequency
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, DisableAllDigitalChannels)(void)
{
    bool retVal = true;
#if defined(PS2000A) || defined(PS3000A)
    PICO_STATUS status;
    wstringstream fraStatusText;

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetDigitalPort)), handle, CommonEnum(SCOPE_FAMILY_UT,DIGITAL_PORT0), 0, 0 );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetDigitalPort)( handle, CommonEnum(SCOPE_FAMILY_UT,DIGITAL_PORT0), 0, 0 )))
    {
        if (PICO_NOT_USED != status) // PICO_NOT_USED is returned when the scope has no digital channels.
        {
            fraStatusText << L"WARNING: Failed to disable digital channels 0-7: " << status;
            LogMessage(fraStatusText.str(), FRA_WARNING);
            retVal = false;
        }
    }

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetDigitalPort)), handle, CommonEnum(SCOPE_FAMILY_UT,DIGITAL_PORT1), 0, 0 );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetDigitalPort)( handle, CommonEnum(SCOPE_FAMILY_UT,DIGITAL_PORT1), 0, 0 )))
    {
        if (PICO_NOT_USED != status) // PICO_NOT_USED is returned when the scope has no digital channels.
        {
            fraStatusText.clear();
            fraStatusText.str(L"");
            fraStatusText << L"WARNING: Failed to disable digital channels 8-15: " << status;
            LogMessage(fraStatusText.str(), FRA_WARNING);
            retVal = false;
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

#undef BANDWIDTH_LIMITER_ARG
#if defined(PS6000)
#define BANDWIDTH_LIMITER_ARG , PS6000_BW_FULL
#else
#define BANDWIDTH_LIMITER_ARG
#endif

bool CommonMethod(SCOPE_FAMILY_LT, DisableChannel)( PS_CHANNEL channel )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;
    float offset = 0.0;

#if defined(PS4000A)
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetChannel)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                                                   FALSE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))0,
                                                                                   (PICO_CONNECT_PROBE_RANGE)0 ANALOG_OFFSET_ARG
                                                                                   BANDWIDTH_LIMITER_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetChannel)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                           FALSE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))0,
                                                           (PICO_CONNECT_PROBE_RANGE)0 ANALOG_OFFSET_ARG
                                                           BANDWIDTH_LIMITER_ARG )))
#else
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetChannel)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                                                   FALSE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))0,
                                                                                   (CommonEnum(SCOPE_FAMILY_UT,RANGE))0 ANALOG_OFFSET_ARG
                                                                                   BANDWIDTH_LIMITER_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetChannel)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
                                                           FALSE, (CommonEnum(SCOPE_FAMILY_UT,COUPLING))0,
                                                           (CommonEnum(SCOPE_FAMILY_UT,RANGE))0 ANALOG_OFFSET_ARG
                                                           BANDWIDTH_LIMITER_ARG )))
#endif
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
// Purpose: Setup the signal generator to generate a signal with the given amplitude and frequency
//
// Parameters: [in] vPP - voltage peak-to-peak in microvolts
//             [in] frequency - frequency to generate in Hz
//             [out] return - whether the function succeeded
//
// Notes: vPP and offset are ignored for PS3000 series scopes
//        If frequency or vPP is 0.0, sets the generator to 0V DC
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#define PS4000_WAVE_TYPE WAVE_TYPE
#define PS5000_WAVE_TYPE WAVE_TYPE

bool CommonMethod(SCOPE_FAMILY_LT, SetSignalGenerator)( double vPP, double offset, double frequency )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;
#if !defined(PS3000)
    CommonEnum(SCOPE_FAMILY_UT, WAVE_TYPE) waveType;
    if (vPP == 0.0 || frequency == 0.0) // To disable the frequency generator
    {
        waveType = CommonDC(SCOPE_FAMILY_UT);
        offset = 0.0;
    }
    else
    {
        waveType = CommonSine(SCOPE_FAMILY_UT);
    }
#endif
#if defined(PS2000)
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetSigGenBuiltIn)), handle, (int32_t)(offset*1.0e6), (uint32_t)(vPP*1.0e6), waveType, (float)frequency, (float)frequency,
                                                                                         1.0, 1.0, (CommonEnum(SCOPE_FAMILY_UT,SWEEP_TYPE))0, 0 );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetSigGenBuiltIn)( handle, (int32_t)(offset*1.0e6), (uint32_t)(vPP*1.0e6), waveType, (float)frequency, (float)frequency,
                                                                 1.0, 1.0, (CommonEnum(SCOPE_FAMILY_UT,SWEEP_TYPE))0, 0 )))
#elif defined(PS3000)
    UNREFERENCED_PARAMETER(vPP);
    UNREFERENCED_PARAMETER(offset);
    // Truncation of frequency to 0 will disable the generator.  However, for PS3000 this function should never be called with values less than 100 Hz, unless
    // actually attempting to disable.
    int32_t intFreq = saturation_cast<int32_t,double>(frequency);
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, _set_siggen)), handle, CommonSine(SCOPE_FAMILY_UT), intFreq, intFreq, 1.0, 1, 0, 0 );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, _set_siggen)( handle, CommonSine(SCOPE_FAMILY_UT), intFreq, intFreq, 1.0, 1, 0, 0 )))
#else
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetSigGenBuiltIn)), handle, (int32_t)(offset*1.0e6), (uint32_t)(vPP*1.0e6), waveType, (float)frequency, (float)frequency,
                                                                                         1.0, 1.0, (CommonEnum(SCOPE_FAMILY_UT,SWEEP_TYPE))0,
                                                                                         CommonEsOff(SCOPE_FAMILY_UT), 
                                                                                         0, 0, (CommonEnum(SCOPE_FAMILY_UT,SIGGEN_TRIG_TYPE))0,
                                                                                         (CommonEnum(SCOPE_FAMILY_UT,SIGGEN_TRIG_SOURCE))CommonSigGenNone(SCOPE_FAMILY_UT), 0 );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetSigGenBuiltIn)( handle, (int32_t)(offset*1.0e6), (uint32_t)(vPP*1.0e6), waveType, (float)frequency, (float)frequency,
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
        fraStatusText.clear();
        fraStatusText.str(L"");
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
    if (!SetSignalGenerator( 0.0, 0.0, 0.0 ))
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

#if defined (PS4000A) || defined(PS5000A)
#define TIME_UNITS_ARG
#define OVERSAMPLE_ARG
#define SEGMENT_INDEX_ARG , 0
#elif defined(PS2000A) || defined(PS3000A) || defined(PS4000) || defined(PS5000) || defined(PS6000)
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
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, GetTimebase)), handle, 3, 0, NULL, TIME_UNITS_ARG OVERSAMPLE_ARG &maxSamples SEGMENT_INDEX_ARG );
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

#if defined(PS4000A)
#define CONDITIONS_INFO , PS4000A_CLEAR
#else
#define CONDITIONS_INFO
#endif

bool CommonMethod(SCOPE_FAMILY_LT, DisableChannelTriggers)( void )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

#if defined(NEW_PS_DRIVER_MODEL)
    // It's possible that only one of these is necessary.  But it's probably good practice to take some action to disable
    // triggers rather than rely on a scopes default state.
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetTriggerChannelConditions)), handle, NULL, 0 CONDITIONS_INFO );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetTriggerChannelConditions)( handle, NULL, 0 CONDITIONS_INFO)))
    {
        fraStatusText << L"Fatal error: Failed to setup channel trigger conditions: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetTriggerChannelProperties)), handle, NULL, 0, 0, 0 );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetTriggerChannelProperties)( handle, NULL, 0, 0, 0 )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to setup channel trigger properties: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
#else
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, _set_trigger)), handle, CommonEnum(SCOPE_FAMILY_UT, NONE), 0, CommonEnum(SCOPE_FAMILY_UT, RISING), 0, 0 );
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
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, RunBlock)), handle, 0, numSamples, timebase, OVERSAMPLE_ARG timeIndisposedMs, 0,
                                                                                 (CommonReadyCB(SCOPE_FAMILY_LT))lpReady, pParameter );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, RunBlock)( handle, 0, numSamples, timebase, OVERSAMPLE_ARG timeIndisposedMs, 0,
                                                         (CommonReadyCB(SCOPE_FAMILY_LT))lpReady, pParameter)))
#else
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, _run_block)), handle, numSamples, timebase, 1, timeIndisposedMs );
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
        capturing = true;
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

    // Remember number of samples for later operations
    mNumSamples = numSamples;
    buffersDirty = true;

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
    wstringstream fraStatusText;
    wstringstream readyStatusText;
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
                readyStatus = 0;
                // delay with a safety factor of 1.5x and never let it go less than 3 seconds
                while (inst->capturing && delayCounter < max( 3000, ((inst->currentTimeIndisposedMs)*3)/2) &&
                       0 == (readyStatus = CommonApi(SCOPE_FAMILY_LT, _ready)(inst->handle)))
                {
                    //// diagnostic logging
                    readyStatusText.clear();
                    readyStatusText.str(L"");
                    readyStatusText << readyStatus << L" <== " << BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, _ready)) << L"( " << inst->handle << L" );";
                    LogMessage( readyStatusText.str(), PICO_API_CALL );
                    //// diagnostic logging

                    Sleep(100);
                    delayCounter += 100;
                }
                if (readyStatus == 0) // timed out or canceled
                {
                    continue; // Don't call the callback to help avoid races between this expiry and the one in PicoScopeFRA
                }
                if (readyStatus < 0)
                {
                    //// diagnostic logging
                    readyStatusText.clear();
                    readyStatusText.str(L"");
                    readyStatusText << readyStatus << L" <== " << BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, _ready)) << L"( " << inst->handle << L" );";
                    LogMessage( readyStatusText.str(), PICO_API_CALL );
                    //// diagnostic logging

                    status = PICO_DATA_NOT_AVAILABLE;
                }
                else
                {
                    //// diagnostic logging
                    readyStatusText.clear();
                    readyStatusText.str(L"");
                    readyStatusText << readyStatus << L" <== " << BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, _ready)) << L"( " << inst->handle << L" );";
                    LogMessage( readyStatusText.str(), PICO_API_CALL );
                    //// diagnostic logging

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
// Name: Common method SetChannelDesignations
//
// Purpose: Tells the PicoScope object which channel is input and which is output
//
// Parameters: [in] inputChannel - input channel number
//             [in] outputChannel - output channel number
//
// Notes:
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void CommonMethod(SCOPE_FAMILY_LT, SetChannelDesignations)( PS_CHANNEL inputChannel, PS_CHANNEL outputChannel )
{
    mInputChannel = inputChannel;
    mOutputChannel = outputChannel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Methods associated with getting data
// Notes: PS2000 and PS3000 drivers support an aggregation mode that works with "fast streaming"
//        which may be able to mimic block mode (using auto_stop).  None of the PS3000 scopes that
//        support fast streaming also have a signal generator, so are irrelevant.  All of the 
//        compatible PS2000 scopes do support fast streaming.  However, given the relatively 
//        smaller size of the buffers these scopes use, I decided to favor code simplicity over
//        speed and just use block mode which has no aggregation.  The aggregation will be done
//        in software on the PC-side.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(PS2000A) || defined(PS3000A) || defined(PS4000A) || defined(PS5000A)
#define RATIO_MODE_NONE_ARG , CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_NONE)
#define RATIO_MODE_AGGREGATE_ARG , CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_AGGREGATE)
#define SEGMENT_ARG , 0
#elif defined(PS6000)
#define RATIO_MODE_NONE_ARG , CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_NONE)
#define RATIO_MODE_AGGREGATE_ARG , CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_AGGREGATE)
#define SEGMENT_ARG
#elif defined(PS4000) || defined(PS5000)
#define RATIO_MODE_NONE_ARG
#define RATIO_MODE_AGGREGATE_ARG
#define SEGMENT_ARG
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method GetData
//
// Purpose: Gets data from the scope that was captured in block mode.
//
// Parameters: [in] numSamples - number of samples to retrieve
//             [in] startIndex - at what index to start retrieving
//             [in] inputBuffer - buffer to store input samples
//             [in] outputBuffer - buffer to store output samples
//             [out] return - whether the function succeeded
//
// Notes:
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// GetData
bool CommonMethod(SCOPE_FAMILY_LT, GetData)( uint32_t numSamples, uint32_t startIndex,
                                             vector<int16_t>** inputBuffer, vector<int16_t>** outputBuffer )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;
    int16_t overflow;

    // First check for the programming error of not setting the channel designations
    if ( PS_CHANNEL_INVALID == mInputChannel ||
         PS_CHANNEL_INVALID == mOutputChannel )
    {
        fraStatusText << L"Fatal error: invalid internal state: input and/or output channel number invalid.";
        LogMessage( fraStatusText.str() );
        return false;
    }
#if defined(NEW_PS_DRIVER_MODEL)
    uint32_t numSamplesInOut;

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mInputChannel, mInputBuffer.data(),
                                                                                     numSamples SEGMENT_ARG RATIO_MODE_NONE_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mInputChannel, mInputBuffer.data(),
                                                             numSamples SEGMENT_ARG RATIO_MODE_NONE_ARG )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to set input data capture buffer: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mOutputChannel, mOutputBuffer.data(),
                                                                                     numSamples SEGMENT_ARG RATIO_MODE_NONE_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mOutputChannel, mOutputBuffer.data(),
                                                                numSamples SEGMENT_ARG RATIO_MODE_NONE_ARG )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to set output data capture buffer: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    numSamplesInOut = numSamples;
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, GetValues)), handle, startIndex, &numSamplesInOut, 1, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_NONE), 0, &overflow );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, GetValues)( handle, startIndex, &numSamplesInOut, 1, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_NONE), 0, &overflow )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to retrieve data capture buffer(s): " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
#else // !defined(NEW_PS_DRIVER_MODEL)
    if (buffersDirty)
    {
        // Just use block mode without aggregation to get all the data.

        int16_t* buffer[PS_CHANNEL_D+1] = {NULL};

        buffer[mInputChannel] = mInputBuffer.data();
        buffer[mOutputChannel] = mOutputBuffer.data();

        LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, _get_values)), handle, buffer[PS_CHANNEL_A], buffer[PS_CHANNEL_B], buffer[PS_CHANNEL_C], buffer[PS_CHANNEL_D], &overflow, numSamples );
        if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, _get_values)( handle, buffer[PS_CHANNEL_A], buffer[PS_CHANNEL_B], buffer[PS_CHANNEL_C], buffer[PS_CHANNEL_D], &overflow, numSamples )))
        {
            fraStatusText.clear();
            fraStatusText.str(L"");
            fraStatusText << L"Fatal error: Failed to retrieve data capture buffer: " << status;
            LogMessage( fraStatusText.str() );
            retVal = false;
        }
    }
#endif
    *inputBuffer = &mInputBuffer;
    *outputBuffer = &mOutputBuffer;

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method GetCompressedData
//
// Purpose: Gets a compressed form of the data from the scope that was captured in block mode.
//          The compressed form is an aggregation where a range of samples is provided as the min
//          and max value in that range.
//
// Parameters: [in] downsampleTo - number of samples to store in a compressed aggregate buffer
//                                 if 0, it means don't compress, just copy
//             [in] inputCompressedMinBuffer - buffer to store aggregated min input samples
//             [in] outputCompressedMinBuffer - buffer to store aggregated min ouput samples
//             [in] inputCompressedMaxBuffer - buffer to store aggregated max input samples
//             [in] outputCompressedMaxBuffer - buffer to store aggregated max ouput samples
//             [out] return - whether the function succeeded
//
// Notes: For old driver model, need to call GetData first for this to function correctly
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// GetCompressedData
bool CommonMethod(SCOPE_FAMILY_LT, GetCompressedData)( uint32_t downsampleTo,
                                                       vector<int16_t>& inputCompressedMinBuffer, vector<int16_t>& outputCompressedMinBuffer,
                                                       vector<int16_t>& inputCompressedMaxBuffer, vector<int16_t>& outputCompressedMaxBuffer )
{
    bool retVal = true;
    wstringstream fraStatusText;
    uint32_t compressedBufferSize;
    uint32_t initialAggregation;

    // First check for the programming error of not setting the channel designations
    if ( PS_CHANNEL_INVALID == mInputChannel ||
         PS_CHANNEL_INVALID == mOutputChannel )
    {
        fraStatusText << L"Fatal error: invalid internal state: input and/or output channel number invalid.";
        LogMessage( fraStatusText.str() );
        return false;
    }

    // For NEW_PS_DRIVER_MODEL: Get the downsampled (compressed) buffer in one step by using the scope's aggregation function
    // For non-NEW_PS_DRIVER_MODEL: We're calculating an aggregation parameter for our own manual aggregation 

    // Determine the aggregation parameter, rounding up to ensure that the actual size of the compressed buffer is no more than requested
    compressedBufferSize = (downsampleTo == 0 || mNumSamples <= downsampleTo) ? mNumSamples : downsampleTo;
    initialAggregation = mNumSamples / compressedBufferSize;
    if (mNumSamples % compressedBufferSize)
    {
        initialAggregation++;
    }
    // Now compute actual size of aggregate buffer
    compressedBufferSize = mNumSamples / initialAggregation;

    // And allocate the data for the compressed buffers
    inputCompressedMinBuffer.resize(compressedBufferSize);
    outputCompressedMinBuffer.resize(compressedBufferSize);
    inputCompressedMaxBuffer.resize(compressedBufferSize);
    outputCompressedMaxBuffer.resize(compressedBufferSize);

#if defined(NEW_PS_DRIVER_MODEL)
    PICO_STATUS status;
    uint32_t numSamplesInOut;
    int16_t overflow;

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mInputChannel,
                                                                                       inputCompressedMaxBuffer.data(), inputCompressedMinBuffer.data(), 
                                                                                       compressedBufferSize SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mInputChannel,
                                                               inputCompressedMaxBuffer.data(), inputCompressedMinBuffer.data(), 
                                                               compressedBufferSize SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to set input data capture aggregation buffers: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mOutputChannel,
                                                                                       outputCompressedMaxBuffer.data(), outputCompressedMinBuffer.data(),
                                                                                       compressedBufferSize SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mOutputChannel,
                                                               outputCompressedMaxBuffer.data(), outputCompressedMinBuffer.data(), 
                                                               compressedBufferSize SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to set output data capture aggregation buffers: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    numSamplesInOut = compressedBufferSize;
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, GetValues)), handle, 0, &numSamplesInOut, initialAggregation, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_AGGREGATE), 0, &overflow );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, GetValues)( handle, 0, &numSamplesInOut, initialAggregation, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_AGGREGATE), 0, &overflow )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to retrieve compressed data: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
#else // !defined(NEW_PS_DRIVER_MODEL)
    // Manually aggregate, if necessary
    if (compressedBufferSize == mNumSamples) // No need to aggregate, just copy
    {
        // Using explicit copy instead of assignment to avoid unwanted resize
        copy(mInputBuffer.begin(), mInputBuffer.begin() + compressedBufferSize, inputCompressedMinBuffer.begin());
        copy(mInputBuffer.begin(), mInputBuffer.begin() + compressedBufferSize, inputCompressedMaxBuffer.begin());
        copy(mOutputBuffer.begin(), mOutputBuffer.begin() + compressedBufferSize, outputCompressedMinBuffer.begin());
        copy(mOutputBuffer.begin(), mOutputBuffer.begin() + compressedBufferSize, outputCompressedMaxBuffer.begin());
    }
    else
    {
        uint32_t i;
        for (i = 0; i < compressedBufferSize-1; i++)
        {
            auto minMaxIn = minmax_element( &mInputBuffer[i*initialAggregation], &mInputBuffer[(i+1)*initialAggregation] );
            inputCompressedMinBuffer[i] = *minMaxIn.first;
            inputCompressedMaxBuffer[i] = *minMaxIn.second;
            auto minMaxOut = minmax_element( &mOutputBuffer[i*initialAggregation], &mOutputBuffer[(i+1)*initialAggregation] );
            outputCompressedMinBuffer[i] = *minMaxOut.first;
            outputCompressedMaxBuffer[i] = *minMaxOut.second;
        }
        // Handle the last few samples
        auto minMaxIn = minmax_element( &mInputBuffer[i*initialAggregation], &mInputBuffer[mNumSamples] );
        inputCompressedMinBuffer[i] = *minMaxIn.first;
        inputCompressedMaxBuffer[i] = *minMaxIn.second;
        auto minMaxOut = minmax_element( &mOutputBuffer[i*initialAggregation], &mOutputBuffer[mNumSamples] );
        outputCompressedMinBuffer[i] = *minMaxOut.first;
        outputCompressedMaxBuffer[i] = *minMaxOut.second;
    }
#endif
    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method GetPeakValues
//
// Purpose: Gets channel peak data values as well as flags indicating channels in overvoltage 
//          condition (ADC railed)
//
// Parameters: [in] inputPeak - Maximum absolute value of input data captured
//             [in] outputPeak - Maximum absolute value of input data captured
//             [in] inputOv - whether the input channel is over-voltage
//             [in] outputOv - whether the output channel is over-voltage 
//             [out] return - whether the function succeeded
//
// Notes:
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// GetPeakValues
bool CommonMethod(SCOPE_FAMILY_LT, GetPeakValues)( uint16_t& inputPeak, uint16_t& outputPeak, bool& inputOv, bool& outputOv )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;
    int16_t overflow;

    // First check for the programming error of not setting the channel designations
    if ( PS_CHANNEL_INVALID == mInputChannel ||
         PS_CHANNEL_INVALID == mOutputChannel )
    {
        fraStatusText << L"Fatal error: invalid internal state: input and/or output channel number invalid.";
        LogMessage( fraStatusText.str() );
        return false;
    }

#if defined(NEW_PS_DRIVER_MODEL)
    uint32_t numSamplesInOut;

    int16_t inputDataMin;
    int16_t inputDataMax;
    int16_t outputDataMin;
    int16_t outputDataMax;

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mInputChannel,
                                                                                       &inputDataMax, &inputDataMin, 1 SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mInputChannel,
                                                               &inputDataMax, &inputDataMin, 1 SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to set input data capture aggregation buffers for peak/overvoltage detection: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)), handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mOutputChannel,
                                                                                       &outputDataMax, &outputDataMin, 1 SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))mOutputChannel,
                                                               &outputDataMax, &outputDataMin, 1 SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to set output data capture aggregation buffers for peak/overvoltage detection: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    numSamplesInOut = 1;
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, GetValues)), handle, 0, &numSamplesInOut, mNumSamples, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_AGGREGATE), 0, &overflow );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, GetValues)( handle, 0, &numSamplesInOut, mNumSamples, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_AGGREGATE), 0, &overflow )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to retrieve aggregated data for peak/overvoltage detection: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
    else
    {
        inputPeak = max(abs(inputDataMax), abs(inputDataMin));
        outputPeak = max(abs(outputDataMax), abs(outputDataMin));
    }
#else
    // Just use block mode without aggregation to get all the data.

    int16_t* buffer[PS_CHANNEL_D+1] = {NULL};

    buffer[mInputChannel] = mInputBuffer.data();
    buffer[mOutputChannel] = mOutputBuffer.data();

    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, _get_values)), handle, buffer[PS_CHANNEL_A], buffer[PS_CHANNEL_B], buffer[PS_CHANNEL_C], buffer[PS_CHANNEL_D], &overflow, mNumSamples );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, _get_values)( handle, buffer[PS_CHANNEL_A], buffer[PS_CHANNEL_B], buffer[PS_CHANNEL_C], buffer[PS_CHANNEL_D], &overflow, mNumSamples )))
    {
        fraStatusText.clear();
        fraStatusText.str(L"");
        fraStatusText << L"Fatal error: Failed to retrieve data capture buffer in peak/overvoltage detection: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }
    else
    {
        buffersDirty = false;
    }

    // Get peak values
    auto inputMinMax = std::minmax_element(&mInputBuffer[0], &mInputBuffer[mNumSamples]);
    inputPeak = max(abs(*inputMinMax.first), abs(*inputMinMax.second));
    auto outputMinMax = std::minmax_element(&mOutputBuffer[0], &mOutputBuffer[mNumSamples]);
    outputPeak = max(abs(*outputMinMax.first), abs(*outputMinMax.second));

#endif

    // Decode overflow
    inputOv = ((overflow & 1<<mInputChannel) != 0);
    outputOv = ((overflow & 1<<mOutputChannel) != 0);

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method ChangePower
//
// Purpose: Change flexible power mode for the scope
//
// Parameters: [out] return - whether the function succeeded
//
// Notes: Only relevant for scopes with flexible power implementations (PS3000A and PS5000A)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, ChangePower)( PICO_STATUS powerState )
{
#if defined(PS3000A) || defined(PS5000A)
    PICO_STATUS status;
    wstringstream fraStatusText;
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, ChangePowerSource)), handle, powerState );
    status = CommonApi(SCOPE_FAMILY_LT, ChangePowerSource)( handle, powerState );
    return (status==PICO_OK);
#else
    return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method CancelCapture
//
// Purpose: Cancel the current capture
//
// Parameters: [out] return - whether the function succeeded
//
// Notes:
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, CancelCapture)(void)
{
    PICO_STATUS status;
    wstringstream fraStatusText;
#if !defined(NEW_PS_DRIVER_MODEL)
    capturing = false;
#endif;
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, Stop)), handle );
    status = CommonApi(SCOPE_FAMILY_LT, Stop)( handle );
    return !(PICO_ERROR(status));
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
    wstringstream fraStatusText;
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, CloseUnit)), handle );
    status = CommonApi(SCOPE_FAMILY_LT, CloseUnit)( handle );
    handle = -1;
    return (PICO_OK == status);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Common method IsUSB3_0Connection
//
// Purpose: Determine if a USB 3.0 scope is on a USB 3.0 port
//
// Parameters: [out] return - true if the connection is USB 3.0
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT, IsUSB3_0Connection)(void)
{
    bool retVal;
    PICO_STATUS status;
    wstringstream fraStatusText;
#if defined(NEW_PS_DRIVER_MODEL)
    int16_t infoStringLength;
#endif
    int8_t usbVersion[16];
    LOG_PICO_API_CALL( BOOST_PP_STRINGIZE(CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)), handle, usbVersion, sizeof(usbVersion) INFO_STRING_LENGTH_ARG, PICO_USB_VERSION );
    if (PICO_ERROR(CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)(handle, usbVersion, sizeof(usbVersion) INFO_STRING_LENGTH_ARG, PICO_USB_VERSION)))
    {
        retVal = false;
    }
    else
    {
        if (!strcmp((char*)usbVersion, "3.0"))
        {
            retVal = true;
        }
        else
        {
            retVal = false;
        }
    }
    return retVal;
}

void CommonMethod(SCOPE_FAMILY_LT, LogPicoApiCall)(wstringstream& fraStatusText)
{
    // If there is a trailing ", " remove it
    size_t length = fraStatusText.str().length();
    if (!fraStatusText.str().compare(length-2, 2, L", "))
    {
        fraStatusText.seekp( -2, ios_base::end );
    }

    fraStatusText << L" );";
    LogMessage( fraStatusText.str(), PICO_API_CALL );
}

template <typename First, typename... Rest> void CommonMethod(SCOPE_FAMILY_LT, LogPicoApiCall)( wstringstream& fraStatusText, First first, Rest... rest)
{
    if (fraStatusText.str().empty()) // first is the function name
    {
        fraStatusText << first << L"( ";
    }
    else // first is a parameter
    {
        fraStatusText << first << L", ";
    }
    LogPicoApiCall( fraStatusText, rest... );
}

#undef GetUnitInfo
#undef SetChannel
#undef SetSigGenBuiltIn
#undef GetTimebase
#undef SetTriggerChannelConditions
#undef CloseUnit
