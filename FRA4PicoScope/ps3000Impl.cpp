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
// Module: ps3000Impl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "utility.h"
#include "ps3000.h"
#include "picoStatus.h"
#include "ps3000Impl.h"
#include "StatusLog.h"

typedef enum enPS3000Coupling
{
    PS3000_AC,
    PS3000_DC
} PS3000_COUPLING;

#define PS3000
#include "psCommonImpl.cpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps3000Impl::GetTimebase
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
bool ps3000Impl::GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase )
{
    bool retVal = false;
    double maxFrequency;
    uint32_t maxTimebase;
    uint32_t minTimebase;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        if (model == PS3205 || model == PS3206)
        {
            switch (model)
            {
                case PS3205:
                    maxFrequency = 100e6;
                    maxTimebase = PS3205_MAX_TIMEBASE;
                    minTimebase = 0;
                    break;
                case PS3206:
                    maxFrequency = 200e6;
                    maxTimebase = PS3206_MAX_TIMEBASE;
                    minTimebase = 1;
                    break;
            }

            *timebase = saturation_cast<uint32_t,double>(log(maxFrequency/desiredFrequency) / M_LN2); // ps3000pg.en r4 p27; log2(n) implemented as log(n)/log(2)

            // Bound to maxTimebase
            // Doing this step in integer space to avoid potential for impossibility to reach maxTimebase caused by floating point precision issues
            *timebase = min(*timebase, maxTimebase);
            // Bound to minTimebase
            *timebase = max(*timebase, minTimebase);

            retVal = GetFrequencyFromTimebase(*timebase, *actualFrequency);
        }
    }

    return retVal;
}

bool ps3000Impl::GetFrequencyFromTimebase(uint32_t timebase, double &frequency)
{
    bool retVal = false;
    double maxFrequency;

    if (model == PS3205 || model == PS3206)
    {
        switch (model)
        {
            case PS3205:
                maxFrequency = 100e6;
                break;
            case PS3206:
                maxFrequency = 200e6;
                break;
        }

        frequency = maxFrequency / (double)(1 << timebase);
        retVal = true;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps3000Impl::InitializeScope
//
// Purpose: Initialize scope/family-specific implementation details.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool ps3000Impl::InitializeScope(void)
{
    bool retVal;

    if (model == PS3205)
    {
        defaultTimebaseNoiseRejectMode = 0; // On the PS3205, 100 MS/s is timebase 0
        minTimebase = 0;
        maxTimebase = PS3205_MAX_TIMEBASE;
    }
    if (model == PS3206)
    {
        defaultTimebaseNoiseRejectMode = 1; // On the PS3206, 100 MS/s is timebase 1
        minTimebase = 1;
        maxTimebase = PS3206_MAX_TIMEBASE;
    }

    signalGeneratorPrecision = 25.0e6 / (1<<28); // Per conversation related to support ticket TS00062849

    minRange = (PS_RANGE)PS3000_100MV;
    maxRange = (PS_RANGE)PS3000_20V;

    retVal = InitStatusChecking();

    return retVal;
}