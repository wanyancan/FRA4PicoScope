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

const RANGE_INFO_T ps4000Impl::rangeInfo[] =
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
    bool retVal = true;
    double fTimebase;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        if (model == PS4262)
        {
            fTimebase = ((10.0e6/(desiredFrequency)) - 1.0); // ps4000pg.en r8 p17
            if (fTimebase <= (double)(1<<30)) // limit is 2^30
            {
                *timebase = (uint32_t)fTimebase;
                *actualFrequency = 10.0e6 / ((double)(*timebase + 1)); // ps4000pg.en r8 p17
            }
        }
        else if (model == PS4226 || model == PS4227)
        {
            if (desiredFrequency >15625000.0)
            {
                *timebase = (uint32_t)(log(250.0e6/desiredFrequency) / M_LN2); // ps4000pg.en r8 p17; log2(n) implemented as log(n)/log(2)
                if (*timebase == 0 && model == PS4226)
                {
                    *timebase = 1; // PS4226 can't use timebase 0
                }
                *actualFrequency = 250.0e6 / (double)(1<<(*timebase));
            }
            else
            {
                fTimebase = ((31250000.0/(desiredFrequency)) + 2.0); // ps4000pg.en r8 p17
                if (fTimebase <= (double)(1<<30)) // limit is 2^30
                {
                    *timebase = (uint32_t)fTimebase;
                    *actualFrequency = 250.0e6 / ((double)(*timebase - 2)); // ps4000pg.en r8 p17
                }
                else
                {
                    retVal = false;
                }
            }
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
    bool retVal = true;

    maxRange = (PS_RANGE)10;

    if (model == PS4262)
    {
        minRange = (PS_RANGE)0;
        timebaseNoiseRejectMode = 15; // for PS4262 => 625 kHz, approximately 3x HW BW limiter
        fSampNoiseRejectMode = 625.0e3; // for PS4262 - approximately 3x HW BW limiter
        signalGeneratorPrecision = 192.0e3 / (double)UINT32_MAX;
    }
    else if (model == PS4226)
    {
        minRange = (PS_RANGE)2; // +/- 50mV
        timebaseNoiseRejectMode = 1; // for PS4226 => 125 MHz
        fSampNoiseRejectMode = 125.0e6;
        signalGeneratorPrecision = 20.0e6 / (double)UINT32_MAX;
    }
    else if (model == PS4227)
    {
        minRange = (PS_RANGE)2; // +/- 50mV
        timebaseNoiseRejectMode = 0; // for PS4227 => 250 MHz
        fSampNoiseRejectMode = 250.0e6;
        signalGeneratorPrecision = 20.0e6 / (double)UINT32_MAX;
    }
    else
    {
        retVal = false;
    }

    return retVal;
}