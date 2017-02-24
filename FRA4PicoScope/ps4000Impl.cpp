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
// Module: ps4000Impl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "utility.h"
#include "ps4000Api.h"
#include "picoStatus.h"
#include "ps4000Impl.h"
#include "StatusLog.h"

typedef enum enPS4000Coupling
{
    PS4000_AC,
    PS4000_DC
} PS4000_COUPLING;

typedef SWEEP_TYPE PS4000_SWEEP_TYPE;

typedef SIGGEN_TRIG_TYPE PS4000_SIGGEN_TRIG_TYPE;
typedef SIGGEN_TRIG_SOURCE PS4000_SIGGEN_TRIG_SOURCE;

const int PS4000_SIGGEN_NONE = SIGGEN_NONE;
const int PS4000_ES_OFF = PS4000_OP_NONE;
const int PS4000_RATIO_MODE_NONE = RATIO_MODE_NONE;
const int PS4000_RATIO_MODE_AGGREGATE = RATIO_MODE_AGGREGATE;

#define PS4000
#include "psCommonImpl.cpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps4000Impl::GetTimebase
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

bool ps4000Impl::GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase )
{
    bool retVal = false;
    double fTimebase;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        if (model == PS4262)
        {
            fTimebase = max(0.0, ((10.0e6/(desiredFrequency)) - 1.0)); // ps4000pg.en r8 p17
            fTimebase = min(fTimebase, (double)((uint32_t)1<<30) - 1.0); // limit is 2^30-1
            *timebase = (uint32_t)fTimebase;
            retVal = GetFrequencyFromTimebase(*timebase, *actualFrequency);
        }
        else if (model == PS4226 || model == PS4227)
        {
            if (desiredFrequency > 15625000.0)
            {
                *timebase = saturation_cast<uint32_t,double>(log(250.0e6/desiredFrequency) / M_LN2); // ps4000pg.en r8 p17; log2(n) implemented as log(n)/log(2)
                if (*timebase == 0 && model == PS4226)
                {
                    *timebase = 1; // PS4226 can't use timebase 0
                }
                retVal = GetFrequencyFromTimebase(*timebase, *actualFrequency);
            }
            else
            {
                fTimebase = ((31250000.0/(desiredFrequency)) + 2.0); // ps4000pg.en r8 p17
                fTimebase = min(fTimebase, (double)((uint32_t)1<<30) - 1.0); // limit is 2^30-1
                *timebase = (uint32_t)fTimebase;
                *timebase = max( *timebase, 4 ); // make sure it's at least 4
                retVal = GetFrequencyFromTimebase(*timebase, *actualFrequency);
            }
        }
    }

    return retVal;

}

bool ps4000Impl::GetFrequencyFromTimebase(uint32_t timebase, double &frequency)
{
    bool retVal = false;

    if (model == PS4262)
    {
        frequency = 10.0e6 / ((double)(timebase + 1)); // ps4000pg.en r8 p17
        retVal = true;
    }
    else if (model == PS4226 || model == PS4227)
    {
        if (timebase <= 3)
        {
            frequency = 250.0e6 / (double)(1<<(timebase));
            retVal = true;
        }
        else
        {
            frequency = 31250000.0 / ((double)(timebase - 2)); // ps4000pg.en r8 p17
            retVal = true;
        }
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps4000Impl::InitializeScope
//
// Purpose: Initialize scope/family-specific implementation details.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ps4000Impl::InitializeScope(void)
{
    maxRange = (PS_RANGE)PS4000_20V;

    if (model == PS4262)
    {
        minRange = (PS_RANGE)PS4000_10MV;
        defaultTimebaseNoiseRejectMode = 15; // for PS4262 => 625 kHz, approximately 3x HW BW limiter
        minTimebase = 0;
        signalGeneratorPrecision = 192.0e3 / (double)UINT32_MAX;
    }
    else if (model == PS4226)
    {
        minRange = (PS_RANGE)PS4000_50MV; // +/- 50mV
        defaultTimebaseNoiseRejectMode = 1; // for PS4226 => 125 MHz
        minTimebase = 1;
        signalGeneratorPrecision = 20.0e6 / (double)UINT32_MAX;
    }
    else if (model == PS4227)
    {
        minRange = (PS_RANGE)PS4000_50MV; // +/- 50mV
        defaultTimebaseNoiseRejectMode = 0; // for PS4227 => 250 MHz
        minTimebase = 0;
        signalGeneratorPrecision = 20.0e6 / (double)UINT32_MAX;
    }

    maxTimebase = ((uint32_t)1<<30) - 1;

    return true;
}