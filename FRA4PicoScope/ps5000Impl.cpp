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
// Module: ps5000Impl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "utility.h"
#include "ps5000Api.h"
#include "picoStatus.h"
#include "ps5000Impl.h"
#include "StatusLog.h"

typedef enum enPS5000Coupling
{
    PS5000_AC,
    PS5000_DC
} PS5000_COUPLING;

typedef SWEEP_TYPE PS5000_SWEEP_TYPE;
typedef SIGGEN_TRIG_TYPE PS5000_SIGGEN_TRIG_TYPE;
typedef SIGGEN_TRIG_SOURCE PS5000_SIGGEN_TRIG_SOURCE;

const int PS5000_SIGGEN_NONE = SIGGEN_NONE;
const int PS5000_ES_OFF = FALSE;
const int PS5000_RATIO_MODE_NONE = RATIO_MODE_NONE;
const int PS5000_RATIO_MODE_AGGREGATE = RATIO_MODE_AGGREGATE;

#define PS5000
#include "psCommonImpl.cpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps5000Impl::GetTimebase
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

bool ps5000Impl::GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase )
{
    bool retVal = true;
    double fTimebase;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        if (desiredFrequency > 125.0e6)
            {
                *timebase = saturation_cast<uint32_t,double>(log(1.0e9/desiredFrequency) / M_LN2); // ps5000pg.en p16; log2(n) implemented as log(n)/log(2)
                *actualFrequency = 1.0e9 / (double)(1<<(*timebase));
            }
            else
            {
                fTimebase = ((125.0e6/(desiredFrequency)) + 2.0); // ps5000pg.en p16
                *timebase = saturation_cast<uint32_t,double>(fTimebase);
                *timebase = max( *timebase, 3 ); // Guarding against potential of float precision issues leading to divide by 0
                *actualFrequency = 125.0e6 / ((double)(*timebase - 2)); // ps5000pg.en p16
            }
    }
    else
    {
        retVal = false;
    }

    return retVal;
}

bool ps5000Impl::GetFrequencyFromTimebase(uint32_t timebase, double &frequency)
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps5000aImpl::InitializeScope
//
// Purpose: Initialize scope/family-specific implementation details.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ps5000Impl::InitializeScope(void)
{
    timebaseNoiseRejectMode = 0;
    fSampNoiseRejectMode = 1.0e9;

    signalGeneratorPrecision = 125.0e6 / (double)UINT32_MAX;

    minRange = (PS_RANGE)PS5000_100MV;
    maxRange = (PS_RANGE)PS5000_20V;

    return true;
}