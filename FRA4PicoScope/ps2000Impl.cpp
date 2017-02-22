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
// Module: ps2000Impl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "utility.h"
#include "ps2000.h"
#include "picoStatus.h"
#include "ps2000Impl.h"
#include "StatusLog.h"

typedef enum enPS2000Coupling
{
    PS2000_AC,
    PS2000_DC
} PS2000_COUPLING;

#define PS2000
#include "psCommonImpl.cpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps2000Impl::GetTimebase
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
bool ps2000Impl::GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase )
{
    bool retVal = false;
    double maxFrequency;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        if (model == PS2203 || model == PS2204 || model == PS2205 || model == PS2204A || model == PS2205A)
        {
            switch (model)
            {
                case PS2203:
                    maxFrequency = 40e6;
                    break;
                case PS2204:
                case PS2204A:
                    maxFrequency = 100e6;
                    break;
                case PS2205: 
                case PS2205A:
                    maxFrequency = 200e6;
                    break;
            }

            *timebase = saturation_cast<uint32_t,double>(log(maxFrequency/desiredFrequency) / M_LN2); // ps2000pg.en r10 p20; log2(n) implemented as log(n)/log(2)

            // Bound to PS2200_MAX_TIMEBASE
            // Doing this step in integer space to avoid potential for impossibility to reach PS2200_MAX_TIMEBASE caused by floating point precision issues
            *timebase = min(*timebase, PS2200_MAX_TIMEBASE);
            // Bound to 1, because none of these scopes can do the maximum frequency with both channels enabled
            *timebase = max(*timebase, 1);

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

bool ps2000Impl::GetFrequencyFromTimebase(uint32_t timebase, double &frequency)
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps2000Impl::InitializeScope
//
// Purpose: Initialize scope/family-specific implementation details.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool ps2000Impl::InitializeScope(void)
{
    bool retVal = true;

    timebaseNoiseRejectMode = 1; // Because two channels are used, the maximum available frequency 
                                 // is half the scope's absolute maximum frequency => timebase=1

    switch (model)
    {
        case PS2202:
        // Let the 2202 initialize, but it is incompatible because it has no signal generator.
        break;
        case PS2203:
            fSampNoiseRejectMode = 20e6;
            break;
        case PS2204: 
        case PS2204A:
            fSampNoiseRejectMode = 50e6;
            break;
        case PS2205: 
        case PS2205A:
            fSampNoiseRejectMode = 100e6;
            break;
        default:
            retVal = false;
            break;
    }

    signalGeneratorPrecision = 48.0e6 / (double)UINT32_MAX;

    minRange = (PS_RANGE)PS2000_50MV;
    maxRange = (PS_RANGE)PS2000_20V;

    if(retVal)
    {
        retVal = InitStatusChecking();
    }

    return retVal;
}