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

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        if (model == PS3205 || model == PS3206)
        {
            switch (model)
            {
                case PS3205:
                    maxFrequency = 100e6; // with 2 channels enabled
                    maxTimebase = PS3205_MAX_TIMEBASE;
                    break;
                case PS3206:
                    maxFrequency = 100e6; // with 2 channels enabled
                    maxTimebase = PS3206_MAX_TIMEBASE;
                    break;
            }

            *timebase = saturation_cast<uint32_t,double>(log(maxFrequency/desiredFrequency) / M_LN2); // ps3000pg.en r4 p27; log2(n) implemented as log(n)/log(2)

            // Bound to (0, maxTimebase)
            // Doing this step in integer space to avoid potential for impossibility to reach maxTimebase caused by floating point precision issues
            *timebase = min(*timebase, maxTimebase);

            *actualFrequency = maxFrequency / (double)(1 << *timebase);
            retVal = true;
        }
        else
        {
            retVal = false;
        }
    }
    else
    {
        retVal = false;
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

    timebaseNoiseRejectMode = 0; // All PS3000 scopes support timebase 0 which is the fastest sampling
    fSampNoiseRejectMode = 100e6; // Both compatible scopes support 100 MS/s

    signalGeneratorPrecision = 1.0; // Because the frequency passed to ps3000_set_siggen is an integer

    minRange = (PS_RANGE)PS3000_100MV;
    maxRange = (PS_RANGE)PS3000_20V;

    retVal = InitStatusChecking();

    return retVal;
}