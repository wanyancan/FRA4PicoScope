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
// Module: ps2000aImpl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "ps2000aApi.h"
#include "picoStatus.h"
#include "ps2000aImpl.h"
#include "StatusLog.h"

const RANGE_INFO_T ps2000aImpl::rangeInfo[] =
{
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

#define PS2000A
#include "psCommonImpl.cpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps2000aImpl::GetTimebase
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

bool ps2000aImpl::GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase )
{
    bool retVal = true;
    double fTimebase;

    if (model == PS2206 || model == PS2206A)
    {
        if (desiredFrequency > 62.5e6)
        {
            *timebase = (uint32_t)(log(500.0e6/desiredFrequency) / M_LN2); // ps2000apg.en r6 p16; log2(n) implemented as log(n)/log(2)
            *actualFrequency = 500.0e6 / (double)(1<<(*timebase));
        }
        else
        {
            fTimebase = ((62.5e6/(desiredFrequency)) + 2.0); // ps2000apg.en r6 p16
            if (fTimebase <= (double)UINT32_MAX)
            {
                *timebase = (uint32_t)fTimebase;
                *actualFrequency = 62.5e6 / ((double)(*timebase - 2)); // ps2000apg.en r6 p16
            }
            else
            {
                retVal = false;
            }
        }
    }
    else if (model == PS2207 || model == PS2207A || model == PS2208 || model == PS2208A)
    {
        if (desiredFrequency > 125.0e6)
        {
            *timebase = (uint32_t)(log(1.0e9/desiredFrequency) / M_LN2); // ps2000apg.en r6 p16; log2(n) implemented as log(n)/log(2)
            *actualFrequency = 1.0e9 / (double)(1<<(*timebase));
        }
        else
        {
            fTimebase = ((125.0e6/(desiredFrequency)) + 2.0); // ps2000apg.en r6 p16
            if (fTimebase <= (double)UINT32_MAX)
            {
                *timebase = (uint32_t)fTimebase;
                *actualFrequency = 125.0e6 / ((double)(*timebase - 2)); // ps2000apg.en r6 p16
            }
            else
            {
                retVal = false;
            }
        }
    }
    else if (model == PS2205MSO)
    {
        fTimebase = 100.0e6 / desiredFrequency;
        if (fTimebase <= (double)UINT32_MAX)
        {
            *timebase = (uint32_t)fTimebase;
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

    if (*timebase == 0)
    {
        *timebase = 1; // None of the 2000A scopes can use timebase 0 with two channels enabled
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps2000aImpl::InitializeScope
//
// Purpose: Initialize scope/family-specific implementation details.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ps2000aImpl::InitializeScope(void)
{

    bool retVal = true;
    timebaseNoiseRejectMode = 1;

    if (model == PS2206 || model == PS2206A)
    {
        fSampNoiseRejectMode = 250.0e6;
        signalGeneratorPrecision = 20.0e6 / (double)UINT32_MAX;
    }
    else if (model == PS2207 || model == PS2207A || model == PS2208 || model == PS2208A)
    {
        fSampNoiseRejectMode = 500.0e6;
        signalGeneratorPrecision = 20.0e6 / (double)UINT32_MAX;
    }
    else if (model == PS2205MSO)
    {
        fSampNoiseRejectMode = 100.0e6;
        signalGeneratorPrecision = 2.0e6 / (double)UINT32_MAX;
    }
    else
    {
        retVal = false;
    }

    minRange = (PS_RANGE)0;
    maxRange = (PS_RANGE)8;

    return retVal;
}