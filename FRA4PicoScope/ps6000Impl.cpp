//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frequency Response Analyzer for PicoScope
//
// Copyright (c) 2014, 2015 by Aaron Hexamer
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
// Module: ps6000Impl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "utility.h"
#include "ps6000Api.h"
#include "picoStatus.h"
#include "ps6000Impl.h"
#include "StatusLog.h"

#define PS6000
#include "psCommonImpl.cpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps6000Impl::GetTimebase
//
// Purpose: Get a timebase from a desired frequency, rounding such that the frequency is at least
//          as high as requested, if possible.
//
// Parameters: [in] desiredFrequency: caller's requested frequency in Hz
//             [out] actualFrequency: the frequency corresponding to the returned timebase.
//             [out] timebase: the timebase that will achieve the requested freqency or greater
//
// Notes:
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ps6000Impl::GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase )
{
    bool retVal = false;
    double fTimebase;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        if (desiredFrequency > 156250000.0)
        {
            *timebase = saturation_cast<uint32_t,double>(log(5.0e9/desiredFrequency) / M_LN2); // ps6000pg.en r9 p19; log2(n) implemented as log(n)/log(2)
            // Now limit the sampling rate based on configuration
            if (((mInputChannel == PS_CHANNEL_A || mInputChannel == PS_CHANNEL_B) &&
                 (mOutputChannel == PS_CHANNEL_A || mOutputChannel == PS_CHANNEL_B)) ||
                ((mInputChannel == PS_CHANNEL_C || mInputChannel == PS_CHANNEL_D) &&
                 (mOutputChannel == PS_CHANNEL_C || mOutputChannel == PS_CHANNEL_D)))
            {
                *timebase = max(*timebase, 2); // If both channels are from same block, cannot use timebase 1 (2.5GS/s)
            }
            else
            {
                *timebase = max(*timebase, 1); // With two channels on, cannot use timebase 0 (5GS/s)
            }
        }
        else
        {
            fTimebase = ((156250000.0/(desiredFrequency)) + 4.0); // ps6000pg.en r9 p19
            *timebase = saturation_cast<uint32_t,double>(fTimebase);
            *timebase = max( *timebase, 5 ); // Guarding against potential of float precision issues leading to divide by 0
        }
        retVal = GetFrequencyFromTimebase(*timebase, *actualFrequency);
    }

    return retVal;
}

bool ps6000Impl::GetFrequencyFromTimebase(uint32_t timebase, double &frequency)
{
    bool retVal = false;

    if (timebase >= minTimebase && timebase <= maxTimebase)
    {
        if (timebase <= 4)
        {
            frequency = 5.0e9 / (double)(1<<(timebase));
        }
        else
        {
            frequency = 156250000.0 / ((double)(timebase - 4)); // ps6000pg.en r9 p19
        }
        retVal = true;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps6000Impl::InitializeScope
//
// Purpose: Initialize scope/family-specific implementation details.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ps6000Impl::InitializeScope(void)
{
    if (model == PS6407)
    {
        // Encode for the best case, and we'll adjust in the GetNoiseRejectMode[SampleRate|Timebase] 
        // functions when both channels are from same block.
        defaultTimebaseNoiseRejectMode = 1;
    }
    else
    {
        // A bandwidth limiter is available on these scope and will be used.
        defaultTimebaseNoiseRejectMode = 6;
    }

    minTimebase = 1;
    maxTimebase = (std::numeric_limits<uint32_t>::max)();

    signalGeneratorPrecision = 200.0e6 / (double)UINT32_MAX;

    if (model == PS6407)
    {
        minRange = maxRange = PS6000_100MV;
    }
    else
    {
        minRange = (PS_RANGE)PS6000_50MV;
        maxRange = (PS_RANGE)PS6000_20V;
    }

    return true;
}