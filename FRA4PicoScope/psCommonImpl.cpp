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
#if defined(PS2000A)
#define SCOPE_FAMILY_LT 2000a
#define SCOPE_FAMILY_UT 2000A
#elif defined(PS4000)
#define SCOPE_FAMILY_LT 4000
#define SCOPE_FAMILY_UT 4000
#elif defined(PS5000A)
#define SCOPE_FAMILY_LT 5000a
#define SCOPE_FAMILY_UT 5000A
#endif

#define CommonCtor(FM) xCommonCtor(FM)
#define xCommonCtor(FM) ps##FM##Impl::ps##FM##Impl

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

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

CommonCtor(SCOPE_FAMILY_LT)( int16_t _handle ) : PicoScope()
{
    handle = _handle;
    initialized = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: 
//
// Purpose: 
//
// Parameters:
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
// Name: 
//
// Purpose: 
//
// Parameters:
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

#if !defined(PS4000)
    (void)CommonApi(SCOPE_FAMILY_LT,MaximumValue)( handle, &maxValue );
#else
    if (model == PS4262)
    {
        maxValue = PS4262_MAX_VALUE;
    }
    else
    {
        maxValue = PS4000_MAX_VALUE;
    }
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
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT,GetModel)( wstring &model )
{
    bool retVal;
    PICO_STATUS status;
    int16_t infoStringLength;
    int8_t scopeModel[32];
    if( PICO_OK != (status = CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)( handle, scopeModel, sizeof(scopeModel), &infoStringLength, PICO_VARIANT_INFO )) )
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
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CommonMethod(SCOPE_FAMILY_LT,GetSerialNumber)( wstring &sn )
{
    bool retVal;
    PICO_STATUS status;
    int16_t infoStringLength;
    int8_t scopeSN[32];
    if( PICO_OK != (status = CommonApi(SCOPE_FAMILY_LT, GetUnitInfo)( handle, scopeSN, sizeof(scopeSN), &infoStringLength, PICO_BATCH_AND_SERIAL )) )
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
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(PS2000A) || defined(PS5000A)
#define ANALOG_OFFSET_ARG , (float)offset
#elif defined(PS4000)
#define ANALOG_OFFSET_ARG
#endif

// SetupChannel
bool CommonMethod(SCOPE_FAMILY_LT, SetupChannel)( PS_CHANNEL channel, PS_COUPLING coupling, PS_RANGE range, float offset )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

    if (0 != (status = CommonApi(SCOPE_FAMILY_LT, SetChannel)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
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
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// DisableChannel
bool CommonMethod(SCOPE_FAMILY_LT, DisableChannel)( PS_CHANNEL channel )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;
    float offset = 0.0;

    if (0 != (status = CommonApi(SCOPE_FAMILY_LT, SetChannel)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))channel,
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
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// SetSignalGenerator
// So far, implemented abstractly for PS5000A and PS4000
bool CommonMethod(SCOPE_FAMILY_LT, SetSignalGenerator)( double vPP, double frequency )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

    if (0 != (status = CommonApi(SCOPE_FAMILY_LT, SetSigGenBuiltIn)( handle, 0, (uint32_t)(vPP*1.0e6), CommonSine(SCOPE_FAMILY_UT), (float)frequency, (float)frequency,
                       1.0, 1.0, (CommonEnum(SCOPE_FAMILY_UT,SWEEP_TYPE))0, CommonEsOff(SCOPE_FAMILY_UT), 0, 0,
                       (CommonEnum(SCOPE_FAMILY_UT,SIGGEN_TRIG_TYPE))0,
                       (CommonEnum(SCOPE_FAMILY_UT,SIGGEN_TRIG_SOURCE))CommonSigGenNone(SCOPE_FAMILY_UT), 0 )))
    {
        fraStatusText << L"Fatal error: Failed to setup stimulus signal: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(PS5000A)
#define OVERSAMPLE_ARG
#elif defined(PS2000A)|| defined(PS4000)
#define OVERSAMPLE_ARG 1,
#endif

// GetMaxSamples
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
    if (0 != (status = CommonApi(SCOPE_FAMILY_LT, GetTimebase)( handle, 3, 0, NULL, OVERSAMPLE_ARG &maxSamples, 0)))
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
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// DisableChannelTriggers
bool CommonMethod(SCOPE_FAMILY_LT, DisableChannelTriggers)( void )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

    // It's possible that only one of these is necessary.  But it's probably good practice to take some action to disable
    // triggers rather than rely on a scopes default state.
    if (0 != (status =  CommonApi(SCOPE_FAMILY_LT, SetTriggerChannelConditions)( handle, NULL, 0 )))
    {
        fraStatusText << L"Fatal error: Failed to setup channel trigger conditions: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    if (0 != (status =  CommonApi(SCOPE_FAMILY_LT, SetTriggerChannelProperties)( handle, NULL, 0, 0, 0 )))
    {
        fraStatusText << L"Fatal error: Failed to setup channel trigger properties: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// RunBlock
bool CommonMethod(SCOPE_FAMILY_LT, RunBlock)( int32_t numSamples, uint32_t timebase, int32_t *timeIndisposedMs, psBlockReady lpReady, void *pParameter )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

    // Setup block mode
    if (0 != (status = CommonApi(SCOPE_FAMILY_LT, RunBlock)( handle, 0, numSamples, timebase, OVERSAMPLE_ARG timeIndisposedMs, 0,
                       (CommonReadyCB(SCOPE_FAMILY_LT))lpReady, pParameter)))
    {
        fraStatusText << L"Fatal error: Failed to start channel data capture: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(PS2000A) || defined(PS5000A)
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
        vector<int16_t>& inputCompressedMaxBuffer, vector<int16_t>& outputCompressedMaxBuffer, int16_t& inputAbsMax, int16_t& outputAbsMax,
        bool* inputOv, bool* outputOv )
{
    PICO_STATUS status;
    bool retVal = true;
    wstringstream fraStatusText;

    uint32_t compressedBufferSize;
    uint32_t initialAggregation;
    uint32_t numSamplesInOut;
    int16_t overflow;

    // First, determine whether the channels are overvoltage and get the downsampled (compressed) buffer by using aggregation

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

    if (0 != (status = CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))inputChannel,
                       inputCompressedMaxBuffer.data(), inputCompressedMinBuffer.data(), compressedBufferSize SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG )))
    {
        fraStatusText << L"Fatal error: Failed to set input data capture buffer: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    if (0 != (status = CommonApi(SCOPE_FAMILY_LT, SetDataBuffers)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))outputChannel,
                       outputCompressedMaxBuffer.data(), outputCompressedMinBuffer.data(), compressedBufferSize SEGMENT_ARG RATIO_MODE_AGGREGATE_ARG )))
    {
        fraStatusText << L"Fatal error: Failed to set output data capture buffer: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    numSamplesInOut = compressedBufferSize;
    if (0 != (status = CommonApi(SCOPE_FAMILY_LT, GetValues)( handle, 0, &numSamplesInOut, initialAggregation, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_AGGREGATE), 0, &overflow )))
    {
        fraStatusText << L"Fatal error: Failed to retrieve data capture buffer for OV detection: " << status;
        LogMessage( fraStatusText.str() );
        retVal = false;
    }

    // Decode overflow
    *inputOv = ((overflow & 1<<inputChannel) != 0);
    *outputOv = ((overflow & 1<<outputChannel) != 0);

    if (compressedBufferSize == numSamples)
    {
        // Simply copy the aggregated samples to the full buffer. It doesn't matter which since they should be the same.
        for (uint32_t i = 0; i < inputCompressedMaxBuffer.size(); i++)
        {
            inputFullBuffer[i] = inputCompressedMaxBuffer[i];
        }
        for (uint32_t i = 0; i < outputCompressedMaxBuffer.size(); i++)
        {
            outputFullBuffer[i] = outputCompressedMaxBuffer[i];
        }
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
            if (0 != (status = CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))inputChannel, inputFullBuffer.data(),
                               numSamples SEGMENT_ARG RATIO_MODE_NONE_ARG )))
            {
                fraStatusText << L"Fatal error: Failed to set input data capture buffer: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }
        else
        {
            if (0 != (status = CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))inputChannel, NULL,
                               0 SEGMENT_ARG RATIO_MODE_NONE_ARG )))
            {
                fraStatusText << L"Fatal error: Failed to set input data capture buffer: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }

        if (!(*outputOv))
        {
            if (0 != (status = CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))outputChannel, outputFullBuffer.data(),
                               numSamples SEGMENT_ARG RATIO_MODE_NONE_ARG )))
            {
                fraStatusText << L"Fatal error: Failed to set output data capture buffer: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }
        else
        {
            if (0 != (status = CommonApi(SCOPE_FAMILY_LT,SetDataBuffer)( handle, (CommonEnum(SCOPE_FAMILY_UT,CHANNEL))outputChannel, NULL,
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
            if (0 != (status = CommonApi(SCOPE_FAMILY_LT, GetValues)( handle, 0, &numSamplesInOut, 1, CommonEnum(SCOPE_FAMILY_UT, RATIO_MODE_NONE), 0, &overflow )))
            {
                fraStatusText << L"Fatal error: Failed to retrieve data capture buffer: " << status;
                LogMessage( fraStatusText.str() );
                retVal = false;
            }
        }
    }

    // Get overall min and max values
    inputAbsMax = abs(*max_element( inputCompressedMinBuffer.begin(), inputCompressedMinBuffer.end(), [](int16_t x, int16_t y) { return (abs(y) > abs(x)); } ));
    inputAbsMax = max( inputAbsMax, abs(*max_element( inputCompressedMaxBuffer.begin(), inputCompressedMaxBuffer.end(), [](int16_t x, int16_t y) { return (abs(y) > abs(x)); } )) );

    outputAbsMax = abs(*max_element( outputCompressedMinBuffer.begin(), outputCompressedMinBuffer.end(), [](int16_t x, int16_t y) { return (abs(y) > abs(x)); } ));
    outputAbsMax = max( outputAbsMax, abs(*max_element( outputCompressedMaxBuffer.begin(), outputCompressedMaxBuffer.end(), [](int16_t x, int16_t y) { return (abs(y) > abs(x)); } )) );

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: 
//
// Purpose: 
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// CloseUnit
bool CommonMethod(SCOPE_FAMILY_LT, CloseUnit)(void)
{
    return (PICO_OK == CommonApi(SCOPE_FAMILY_LT, CloseUnit)( handle ));
}



