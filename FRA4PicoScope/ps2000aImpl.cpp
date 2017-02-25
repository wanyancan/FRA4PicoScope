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
#include "utility.h"
#include "ps2000aApi.h"
#include "picoStatus.h"
#include "ps2000aImpl.h"
#include "StatusLog.h"

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
    bool retVal = false;
    double fTimebase;

    if (desiredFrequency != 0.0 && actualFrequency && timebase)
    {
        switch (model)
        {
            // 500 MSs models
            case PS2206:
            case PS2206A:
            case PS2206B:
            case PS2205AMSO:
            case PS2405A:
            {
                if (desiredFrequency > 62.5e6)
                {
                    *timebase = saturation_cast<uint32_t, double>(log(500.0e6 / desiredFrequency) / M_LN2); // ps2000apg.en r6 p16; log2(n) implemented as log(n)/log(2)
                    *timebase = max(*timebase, 1); // None of the 2000A scopes can use timebase 0 with two channels enabled
                }
                else
                {
                    fTimebase = ((62.5e6 / (desiredFrequency)) + 2.0); // ps2000apg.en r6 p16
                    *timebase = saturation_cast<uint32_t, double>(fTimebase);
                    *timebase = max(*timebase, 3); // Guarding against potential of float precision issues leading to divide by 0
                }
                retVal = GetFrequencyFromTimebase(*timebase, *actualFrequency);
                break;
            }
            // 1GSs models
            case PS2207:
            case PS2207A:
            case PS2207B:
            case PS2208:
            case PS2208A:
            case PS2208B:
            case PS2206BMSO:
            case PS2207BMSO:
            case PS2208BMSO:
            case PS2406B:
            case PS2407B:
            case PS2408B:
            {
                if (desiredFrequency > 125.0e6)
                {
                    *timebase = saturation_cast<uint32_t, double>(log(1.0e9 / desiredFrequency) / M_LN2); // ps2000apg.en r6 p16; log2(n) implemented as log(n)/log(2)
                    *timebase = max(*timebase, 1); // None of the 2000A scopes can use timebase 0 with two channels enabled
                }
                else
                {
                    fTimebase = ((125.0e6 / (desiredFrequency)) + 2.0); // ps2000apg.en r6 p16
                    *timebase = saturation_cast<uint32_t, double>(fTimebase);
                    *timebase = max(*timebase, 3); // Guarding against potential of float precision issues leading to divide by 0
                }
                retVal = GetFrequencyFromTimebase(*timebase, *actualFrequency);
                break;
            }
            case PS2205MSO:
            {
                fTimebase = 100.0e6 / desiredFrequency;
                *timebase = saturation_cast<uint32_t, double>(fTimebase);
                *timebase = max(*timebase, 1); // None of the 2000A scopes can use timebase 0 with two channels enabled; also guards against divide by 0
                retVal = GetFrequencyFromTimebase(*timebase, *actualFrequency);
                break;
            }
            default:
                break;
        }
    }

    return retVal;
}

bool ps2000aImpl::GetFrequencyFromTimebase(uint32_t timebase, double &frequency)
{
    bool retVal = false;

    if (timebase >= minTimebase && timebase <= maxTimebase)
    {
        switch (model)
        {
            // 500 MSs models
            case PS2206:
            case PS2206A:
            case PS2206B:
            case PS2205AMSO:
            case PS2405A:
            {
                if (timebase <= 2)
                {
                    frequency = 500.0e6 / (double)(1 << (timebase));
                }
                else
                {
                    frequency = 62.5e6 / ((double)(timebase - 2)); // ps2000apg.en r6 p16
                }
                retVal = true;
                break;
            }
            // 1GSs models
            case PS2207:
            case PS2207A:
            case PS2207B:
            case PS2208:
            case PS2208A:
            case PS2208B:
            case PS2206BMSO:
            case PS2207BMSO:
            case PS2208BMSO:
            case PS2406B:
            case PS2407B:
            case PS2408B:
            {
                if (timebase <= 2)
                {
                    frequency = 1.0e9 / (double)(1 << (timebase));
                }
                else
                {
                    frequency = 125.0e6 / ((double)(timebase - 2)); // ps2000apg.en r6 p16
                }
                retVal = true;
                break;
            }
            case PS2205MSO:
            {
                if (timebase > 0)
                {
                    frequency = 100.0e6 / ((double)timebase); // ps2000apg.en r6 p16
                }
                else
                {
                    frequency = 200.0e6;
                }
                retVal = true;
                break;
            }
            default:
                break;
        }
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
    defaultTimebaseNoiseRejectMode = 1;

    minTimebase = 1;
    maxTimebase = (std::numeric_limits<uint32_t>::max)();

    switch (model)
    {
        case PS2206:
        case PS2206A:
        {
            signalGeneratorPrecision = 20.0e6 / (double)UINT32_MAX;
            break;
        }
        case PS2207:
        case PS2207A:
        case PS2208:
        case PS2208A:
        case PS2408B:
        {
            signalGeneratorPrecision = 20.0e6 / (double)UINT32_MAX;
            break;
        }
        case PS2205MSO:
        {
            signalGeneratorPrecision = 48.0e6 / (double)UINT32_MAX;
            break;
        }
        default:
            retVal = false;
            break;
    }

    if (retVal)
    {
        maxRange = (PS_RANGE)PS2000A_20V;
        switch (model)
        {
            case PS2206B:
            case PS2207B:
            case PS2208B:
            case PS2205AMSO:
            case PS2206BMSO:
            case PS2207BMSO:
            case PS2208BMSO:
            case PS2405A:
            case PS2406B:
            case PS2407B:
            case PS2408B:
            {
                minRange = (PS_RANGE)PS2000A_20MV;
                break;
            }
            case PS2206A:
            case PS2207A:
            case PS2208A:
            case PS2205MSO:
            {
                minRange = (PS_RANGE)PS2000A_50MV;
                break;
            }
            default:
                retVal = false;
                break;
        }
    }

    return retVal;
}