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
// Module: ps3000aImpl.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "utility.h"
#include "ps3000aApi.h"
#include "picoStatus.h"
#include "ps3000aImpl.h"
#include "StatusLog.h"

#define PS3000A
#include "psCommonImpl.cpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ps3000aImpl::GetTimebase
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

bool ps3000aImpl::GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase )
{
    bool retVal = true;
    double fTimebase;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        if (model == PS3204A || model == PS3204B || model == PS3205A || model == PS3205B || model == PS3206A || model == PS3206B)
        {
            if (desiredFrequency > 62.5e6)
            {
                *timebase = saturation_cast<uint32_t,double>(log(500.0e6/desiredFrequency) / M_LN2); // ps3000apg.en r10 p11; log2(n) implemented as log(n)/log(2)
                *timebase = max( *timebase, 1 ); // None of the 3000A scopes can use timebase 0 with two or more channels enabled
                *actualFrequency = 500.0e6 / (double)(1<<(*timebase));
            }
            else
            {
                fTimebase = ((62.5e6/(desiredFrequency)) + 2.0); // ps3000apg.en r10 p11
                *timebase = saturation_cast<uint32_t,double>(fTimebase);
                *timebase = max( *timebase, 3 ); // Guarding against potential of float precision issues leading to divide by 0
                *actualFrequency = 62.5e6 / ((double)(*timebase - 2)); // ps3000apg.en r10 p11
            }
        }
        else if (model == PS3404A || model == PS3404B || model == PS3405A || model == PS3405B || model == PS3406A || model == PS3406B || model == PS3207A || model == PS3207B ||
                 model == PS3203D || model == PS3203DMSO || model == PS3204D || model == PS3204DMSO || model == PS3205D || model == PS3205DMSO || model == PS3206D || model == PS3206DMSO ||
                 model == PS3403D || model == PS3403DMSO || model == PS3404D || model == PS3404DMSO || model == PS3405D || model == PS3405DMSO || model == PS3406D || model == PS3406DMSO)
        {
            if (desiredFrequency > 125.0e6)
            {
                *timebase = saturation_cast<uint32_t,double>(log(1.0e9/desiredFrequency) / M_LN2); // ps3000apg.en r10 p11; log2(n) implemented as log(n)/log(2)
                *timebase = max( *timebase, 1 ); // None of the 3000A scopes can use timebase 0 with two or more channels enabled
                *actualFrequency = 1.0e9 / (double)(1<<(*timebase));
            }
            else
            {
                fTimebase = ((125.0e6/(desiredFrequency)) + 2.0); // ps3000apg.en r10 p11
                *timebase = saturation_cast<uint32_t,double>(fTimebase);
                *timebase = max( *timebase, 3 ); // Guarding against potential of float precision issues leading to divide by 0
                *actualFrequency = 125.0e6 / ((double)(*timebase - 2)); // ps3000apg.en r10 p11
            }
        }
        else if (model == PS3204MSO || model == PS3205MSO || model == PS3206MSO)
        {
            if (desiredFrequency > 125.0e6)
            {
                *timebase = saturation_cast<uint32_t,double>(log(500.0e6/desiredFrequency) / M_LN2); // ps3000apg.en r10 p11; log2(n) implemented as log(n)/log(2)
                *timebase = max( *timebase, 1 ); // None of the 3000A scopes can use timebase 0 with two or more channels enabled
                *actualFrequency = 500.0e6 / (double)(1<<(*timebase));
            }
            else
            {
                fTimebase = ((125.0e6/(desiredFrequency)) + 1.0); // ps3000apg.en r10 p11
                *timebase = saturation_cast<uint32_t,double>(fTimebase);
                *timebase = max( *timebase, 2 ); // Guarding against potential of float precision issues leading to divide by 0
                *actualFrequency = 125.0e6 / ((double)(*timebase - 1)); // ps3000apg.en r10 p11
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
// Name: ps3000aImpl::InitializeScope
//
// Purpose: Initialize scope/family-specific implementation details.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ps3000aImpl::InitializeScope(void)
{

    if (model == PS3204A || model == PS3204B || model == PS3205A || model == PS3205B || model == PS3206A || model == PS3206B || model == PS3204MSO || model == PS3205MSO || model == PS3206MSO)
    {
        timebaseNoiseRejectMode = 1;
        fSampNoiseRejectMode = 250.0e6;
    }
    else if (model == PS3207A || model == PS3207B)
    {
        timebaseNoiseRejectMode = 1;
        fSampNoiseRejectMode = 500.0e6;
    }
    else if (model == PS3404A || model == PS3404B || model == PS3405A || model == PS3405B || model == PS3406A || model == PS3406B ||
             model == PS3203D || model == PS3203DMSO || model == PS3204D || model == PS3204DMSO || model == PS3205D || model == PS3205DMSO || model == PS3206D || model == PS3206DMSO ||
             model == PS3403D || model == PS3403DMSO || model == PS3404D || model == PS3404DMSO || model == PS3405D || model == PS3405DMSO || model == PS3406D || model == PS3406DMSO)
    {
        // These models have a switchable 20 MHz bandwidth limiter
        timebaseNoiseRejectMode = 4;
        fSampNoiseRejectMode = 62500000.0;
    }

    if (model == PS3204A || model == PS3204B || model == PS3205A || model == PS3205B || model == PS3206A || model == PS3206B || model == PS3204MSO || model == PS3205MSO || model == PS3206MSO ||
        model == PS3404A || model == PS3404B || model == PS3405A || model == PS3405B || model == PS3406A || model == PS3406B ||
        model == PS3203D || model == PS3203DMSO || model == PS3204D || model == PS3204DMSO || model == PS3205D || model == PS3205DMSO || model == PS3206D || model == PS3206DMSO ||
        model == PS3403D || model == PS3403DMSO || model == PS3404D || model == PS3404DMSO || model == PS3405D || model == PS3405DMSO || model == PS3406D || model == PS3406DMSO)
    {
        signalGeneratorPrecision = 20.0e6 / (double)UINT32_MAX;
    }
    else if (model == PS3207A || model == PS3207B)
    {
        signalGeneratorPrecision = 100.0e6 / (double)UINT32_MAX;
    }    

    if (model == PS3203D || model == PS3203DMSO || model == PS3204D || model == PS3204DMSO || model == PS3205D || model == PS3205DMSO || model == PS3206D || model == PS3206DMSO ||
        model == PS3403D || model == PS3403DMSO || model == PS3404D || model == PS3404DMSO || model == PS3405D || model == PS3405DMSO || model == PS3406D || model == PS3406DMSO)
    {
        minRange = (PS_RANGE)PS3000A_20MV;
    }
    else
    {
        minRange = (PS_RANGE)PS3000A_50MV;
    }

    maxRange = (PS_RANGE)PS3000A_20V;

    // Save the value set by the scope factory (ScopeSelector)
    numActualChannels = numAvailableChannels;

    if (PICO_POWER_SUPPLY_NOT_CONNECTED == initialPowerState ||
        PICO_USB3_0_DEVICE_NON_USB3_0_PORT == initialPowerState)
    {
        ps3000aChangePowerSource(handle, initialPowerState);
        numAvailableChannels = 2;
    }
    else
    {
        numAvailableChannels = numActualChannels;
    }

    return true;
}