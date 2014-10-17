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
// Module: ps5000aImpl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "utility.h"
#include "ps5000aApi.h"
#include "picoStatus.h"
#include "ps5000aImpl.h"
#include "StatusLog.h"

#define PS5000A
#include "psCommonImpl.cpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps5000aImpl::GetTimebase
//
// Purpose: Get a timebase from a desired frequency, rounding such that the frequency is at least
//          as high as requested, if possible.
//
// Parameters: [in] desiredFrequency: caller's requested frequency in Hz
//             [out] actualFrequency: the frequency corresponding to the returned timebase.
//             [out] timebase: the timebase that will achieve the requested freqency or greater
//
// Notes: Assumes PS5000A is in 14 or 15 bit sampling mode, which it should be for FRA
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ps5000aImpl::GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase )
{
    bool retVal = true;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        *timebase = saturation_cast<uint32_t,double>((125.0e6/desiredFrequency) + 2.0); // ps5000abpg.en r1: p18
        *timebase = max( *timebase, 3 ); // make sure it's at least 3
        *actualFrequency = 125.0e6 / ((double)(*timebase - 2)); // ps5000abpg.en r1: p18
    }
    else
    {
        retVal = false;
    }

    return retVal;
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

bool ps5000aImpl::InitializeScope(void)
{
    timebaseNoiseRejectMode = 4; // for PS5000A => 62.5 MHz approximately 3x HW BW limiter
    fSampNoiseRejectMode = 62500000.0; // for PS5000A - approximately 3x HW BW limiter

    signalGeneratorPrecision = 200.0e6 / (double)UINT32_MAX;

    minRange = (PS_RANGE)PS5000A_10MV;
    maxRange = (PS_RANGE)PS5000A_20V;

    return true;
}