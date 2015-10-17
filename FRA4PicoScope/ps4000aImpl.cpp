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
// Module: ps4000aImpl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "utility.h"
#include "ps4000aApi.h"
#include "picoStatus.h"
#include "ps4000aImpl.h"
#include "StatusLog.h"

#define PS4000A
#include "psCommonImpl.cpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps4000aImpl::GetTimebase
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

bool ps4000aImpl::GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase )
{
    bool retVal = true;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        *timebase = saturation_cast<uint32_t,double>((80.0e6/desiredFrequency) - 1.0); // ps4000apg.en r1: p17
        *actualFrequency = 80.0e6 / ((double)(*timebase + 1)); // ps4000apg.en r1: p17
    }
    else
    {
        retVal = false;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps4000aImpl::InitializeScope
//
// Purpose: Initialize scope/family-specific implementation details.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ps4000aImpl::InitializeScope(void)
{
    timebaseNoiseRejectMode = 0;
    fSampNoiseRejectMode = 80.0e6;

    signalGeneratorPrecision = 80.0e6 / (double)UINT32_MAX;

    minRange = (PS_RANGE)PS4000A_10MV;
    maxRange = (PS_RANGE)PS4000A_20V;

    return true;
}