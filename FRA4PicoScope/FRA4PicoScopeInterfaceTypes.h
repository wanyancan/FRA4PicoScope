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
// Module FRA4PicoScopeInterfaceTypes.h: Types supporting both the FRA4PicoScope Application
//                                       and the FRA4PicoScope API DLL.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

typedef enum
{
    PS_CHANNEL_A,
    PS_CHANNEL_B,
    PS_CHANNEL_C,
    PS_CHANNEL_D,
    PS_CHANNEL_E,
    PS_CHANNEL_F,
    PS_CHANNEL_G,
    PS_CHANNEL_H,
    PS_CHANNEL_INVALID
} PS_CHANNEL;

typedef enum
{
    PS_AC,
    PS_DC,
    PS_DC_1M = PS_DC,
    PS_DC_50R
} PS_COUPLING;

typedef enum
{
    ATTEN_1X,
    ATTEN_10X,
    ATTEN_20X,
    ATTEN_100X,
    ATTEN_200X,
    ATTEN_1000X
} ATTEN_T;

typedef enum
{
    LOW_NOISE,
    HIGH_NOISE
} SamplingMode_T;

typedef enum
{
    OK, // Measurement is acceptable
    HIGHEST_RANGE_LIMIT_REACHED, // Overflow or amplitude too high and already on the highest range
    LOWEST_RANGE_LIMIT_REACHED, // Amplitude too low for good measurement precision and already on lowest range
    CHANNEL_OVERFLOW, // Overflow flag set
    AMPLITUDE_TOO_HIGH, // Amplitude is close enough to full scale that it would be best to increase the range
    AMPLITUDE_TOO_LOW // Amplitude is low enough that range can be decreased to increase the measurement precision.
} AUTORANGE_STATUS_T;

typedef enum
{
    FRA_STATUS_IDLE,
    FRA_STATUS_IN_PROGRESS,
    FRA_STATUS_COMPLETE,
    FRA_STATUS_CANCELED,
    FRA_STATUS_AUTORANGE_LIMIT,
    FRA_STATUS_POWER_CHANGED,
    FRA_STATUS_FATAL_ERROR,
    FRA_STATUS_MESSAGE
} FRA_STATUS_T;

typedef struct
{
    FRA_STATUS_T status;
    union
    {
        struct
        {
            int stepsComplete;
            int numSteps;
        } progress;
        struct
        {
            int stepsComplete;
            int numSteps;
        } cancelPoint;
        struct
        {
            AUTORANGE_STATUS_T inputChannelStatus;
            AUTORANGE_STATUS_T outputChannelStatus;
        } autorangeLimit;
        bool powerState; // false = No Aux DC power
    } statusData;

    std::wstring statusText; // used to encode log or failure messages

    struct
    {
        // Whether to proceed, or cancel the FRA execution.  Should only be
        // used as a response to FRA_AUTORANGE_LIMIT or PICO_POWER_CHANGE so
        // that we don't create a race condition.  Cancel from the UI that's
        // asynchronous (not tied to a specific FRA operation like autorange)
        // is handled separately.
        bool proceed;

        union
        {
            struct
            {
                ATTEN_T inputAtten;
                ATTEN_T outputAtten;
                double inputAmplify;
                double outputAmplify;
            } autorangeAdjustment;
        } response;

    } responseData;

} FRA_STATUS_MESSAGE_T;

typedef bool (*FRA_STATUS_CALLBACK)(FRA_STATUS_MESSAGE_T& pFraStatus);